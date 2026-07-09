#include "dnsprobes.h"

#include <QElapsedTimer>
#include <atomic>
#include <cstring>
#include <mutex>
#include <thread>

#include <wsnet/WSNet.h>
#include <wsnet/WSNetCancelableCallback.h>
#include <wsnet/WSNetDnsRequestResult.h>
#include <wsnet/WSNetDnsResolver.h>
#include <wsnet/WSNetRequestError.h>

#ifdef Q_OS_WIN
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <netdb.h>
    #include <sys/socket.h>
#endif

using namespace wsnet;

namespace {

// Process-wide bound on concurrent detached system-DNS workers. Each worker holds
// a (non-cancelable) getaddrinfo() until the OS resolver returns, so without a cap
// a black-holed resolver plus repeated kNoApiConnectivity runs could pile up an
// unbounded number of stuck threads. Past this cap the system probe is declined
// and reported as timed out — the app-DNS comparison is the higher-signal data
// anyway. One run uses at most 2 workers (A + AAAA).
constexpr int kMaxInFlightSystemDnsWorkers = 8;
std::atomic<int> g_inFlightSystemDnsWorkers{0};

} // namespace

// Back-channel shared (via shared_ptr) between a detached system-DNS worker
// thread and its owning DnsProbes. owner is read/written only under mutex.
struct SystemDnsProbeChannel
{
    std::mutex mutex;
    DnsProbes *owner = nullptr;  // cleared under the lock when DnsProbes is destroyed
};

DnsProbes::DnsProbes(QObject *parent) : QObject(parent)
{
}

DnsProbes::~DnsProbes()
{
    // Cancel any in-flight wsnet lookups so their callbacks become no-ops (the
    // cancelable callback synchronizes with the resolver thread, so this is safe
    // even if a callback is being invoked concurrently).
    for (const auto &handle : cancelHandles_) {
        if (handle)
            handle->cancel();
    }

    // Detach any still-running system-DNS workers: clear the back-pointer under
    // the lock so a getaddrinfo() that returns later drops its result instead of
    // touching this (now-destroyed) object or posting to the engine thread. The
    // detached threads keep their own shared_ptr to the channel, so the channel
    // outlives us; we never block waiting for them.
    for (const auto &channel : systemChannels_) {
        std::lock_guard<std::mutex> lock(channel->mutex);
        channel->owner = nullptr;
    }
}

void DnsProbes::start(const QString &host)
{
    if (started_)
        return;
    started_ = true;

    startAppDnsProbe(host, true);
    startAppDnsProbe(host, false);
    startSystemDnsProbe(host, true);
    startSystemDnsProbe(host, false);
}

void DnsProbes::startAppDnsProbe(const QString &host, bool isV4)
{
    // Marshal the resolver callback (delivered on a wsnet thread) back onto the
    // engine thread, mirroring the pattern in KeepAliveManager.
    auto callback = [this, isV4](std::uint64_t requestId, const std::string &hostname,
                                 std::shared_ptr<WSNetDnsRequestResult> result) {
        Q_UNUSED(requestId);
        Q_UNUSED(hostname);
        QMetaObject::invokeMethod(this, [this, isV4, result] {  // NOLINT: false positive for memory leak
            onAppDnsFinished(isV4, result);
        });
    };

    auto handle = WSNet::instance()->dnsResolver()->lookup(
        host.toStdString(), 0, isV4 ? IpFamily::kIpv4 : IpFamily::kIpv6, callback);
    cancelHandles_.push_back(handle);
}

void DnsProbes::startSystemDnsProbe(const QString &host, bool isV4)
{
    // KNOWN TRADE-OFF: getaddrinfo() is a blocking, non-cancelable POSIX/WSAPI
    // call, so this lookup is NOT bounded by the orchestrator's overall time cap.
    // When the cap fires the diagnostic is finished and this DnsProbes is
    // deleteLater()'d, but a hung system resolver can keep the worker alive until
    // the kernel resolver itself gives up (tens of seconds). The cap therefore
    // bounds the diagnostic run, not this lookup.
    //
    // To make that detachment crash- and leak-safe, the worker is a self-owned,
    // detached std::thread that owns nothing on the engine thread: it reports its
    // result back through a mutex-guarded channel ONLY while DnsProbes is still
    // alive, and otherwise drops it. Because there is no engine-thread QThread
    // object or queued quit/deleteLater, a lookup that returns after this object
    // (and the engine thread) is gone can never touch freed memory or post to a
    // torn-down event loop.
    //
    // Concurrency bound: if too many resolver workers are already stuck,
    // decline this probe rather than spawn yet another stalled thread. Report it
    // as timed out so the run still completes and the log shows the saturation.
    if (g_inFlightSystemDnsWorkers.load(std::memory_order_relaxed) >= kMaxInFlightSystemDnsWorkers) {
        DnsProbeResult &probe = isV4 ? systemDnsV4_ : systemDnsV6_;
        probe.status = ProbeStatus::kTimeout;
        if (++systemFinishedCount_ >= kSystemProbeCount)
            downgradeMissingAaaaAsSkipped(systemDnsV6_, systemDnsV4_);
        emitFinishedIfAllDone();
        return;
    }

    auto channel = std::make_shared<SystemDnsProbeChannel>();
    channel->owner = this;
    systemChannels_.push_back(channel);

    g_inFlightSystemDnsWorkers.fetch_add(1, std::memory_order_relaxed);
    std::thread([channel, host, isV4]() {
        // Decrement the process-wide worker count on every exit path, so a stuck
        // lookup frees its slot the moment getaddrinfo() finally returns.
        struct WorkerCountGuard {
            ~WorkerCountGuard() { g_inFlightSystemDnsWorkers.fetch_sub(1, std::memory_order_relaxed); }
        } workerCountGuard;

        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = isV4 ? AF_INET : AF_INET6;
        hints.ai_socktype = SOCK_STREAM;

        QElapsedTimer timer;
        timer.start();

        struct addrinfo *res = nullptr;
        const int rc = getaddrinfo(host.toUtf8().constData(), nullptr, &hints, &res);
        const qint64 elapsedMs = timer.elapsed();

        int addressCount = 0;
        if (rc == 0) {
            for (const struct addrinfo *node = res; node != nullptr; node = node->ai_next)
                ++addressCount;
        }
        if (res != nullptr)
            freeaddrinfo(res);

        // Marshal the aggregate outcome back to the engine thread, but only while
        // DnsProbes is alive. Holding the lock keeps owner valid across the post:
        // ~DnsProbes blocks on this same mutex before it clears owner and finishes
        // destruction, so the queued call targets a live object (and is dropped by
        // QObject's own removePostedEvents() if destruction follows). The
        // addresses themselves are discarded — only counts/timing leave the thread.
        std::lock_guard<std::mutex> lock(channel->mutex);
        if (channel->owner == nullptr)
            return;
        DnsProbes *owner = channel->owner;
        QMetaObject::invokeMethod(owner, [owner, isV4, rc, elapsedMs, addressCount]() {
            owner->onSystemDnsFinished(isV4, rc == 0, rc, elapsedMs, addressCount);
        });
    }).detach();
}

void DnsProbes::onAppDnsFinished(bool isV4, std::shared_ptr<WSNetDnsRequestResult> result)
{
    DnsProbeResult &probe = isV4 ? appDnsV4_ : appDnsV6_;

    const bool success = result->error()->isSuccess();
    probe.elapsedMs = static_cast<qint64>(result->elapsedMs());

    const std::vector<std::string> ips = result->ips();
    probe.addressCount = static_cast<int>(ips.size());
    probe.status = success ? ProbeStatus::kSuccess : ProbeStatus::kFailure;
    // The wsnet error surface does not expose a numeric resolver code; the status
    // captures success/failure and errorCode is left at its 0 default.

    if (success) {
        // Retain the resolved addresses (not logged) so the orchestrator can drive
        // the forced-IP HTTP probes (section C).
        QStringList &resolved = isV4 ? resolvedV4_ : resolvedV6_;
        resolved.clear();
        for (const std::string &ip : ips)
            resolved << QString::fromStdString(ip);
    }

    if (++appFinishedCount_ >= kAppProbeCount) {
        // Both app probes are in. Downgrade a missing-AAAA "failure" before anyone
        // reads the results, then announce that the app-DNS data (and the resolved
        // addresses that drive the forced-IP HTTP probes) is ready. This is
        // deliberately decoupled from the system-DNS probes: getaddrinfo() is not
        // cancelable, so a hung system lookup must not delay the HTTP evidence.
        downgradeMissingAaaaAsSkipped(appDnsV6_, appDnsV4_);
        emit appDnsFinished();
    }
    emitFinishedIfAllDone();
}

void DnsProbes::onSystemDnsFinished(bool isV4, bool success, int errorCode, qint64 elapsedMs, int addressCount)
{
    DnsProbeResult &probe = isV4 ? systemDnsV4_ : systemDnsV6_;
    probe.status = success ? ProbeStatus::kSuccess : ProbeStatus::kFailure;
    probe.errorCode = errorCode;
    probe.elapsedMs = elapsedMs;
    probe.addressCount = addressCount;

    if (++systemFinishedCount_ >= kSystemProbeCount)
        downgradeMissingAaaaAsSkipped(systemDnsV6_, systemDnsV4_);
    emitFinishedIfAllDone();
}

void DnsProbes::emitFinishedIfAllDone()
{
    if (isFinished())
        emit finished();
}

void DnsProbes::markUnfinishedAsTimedOut()
{
    // All four probes are started in start(); any still kNotRun is in flight (for
    // system DNS, a stuck getaddrinfo()). Report it as timed out, not "n/a". A
    // probe that already reported, was skipped, or was declined at the cap is left
    // untouched.
    const auto mark = [](DnsProbeResult &p) {
        if (p.status == ProbeStatus::kNotRun)
            p.status = ProbeStatus::kTimeout;
    };
    mark(appDnsV4_);
    mark(appDnsV6_);
    mark(systemDnsV4_);
    mark(systemDnsV6_);
}

void DnsProbes::downgradeMissingAaaaAsSkipped(DnsProbeResult &v6, const DnsProbeResult &v4)
{
    // A v6 (AAAA) lookup that failed with no addresses, while the SAME resolver's
    // v4 (A) lookup succeeded, means the host simply has no AAAA record (or the
    // network has no IPv6) — not a DNS failure. Mark it skipped so it is not read
    // as a problem by the classification line or the hint heuristics.
    if (v6.status == ProbeStatus::kFailure && v6.addressCount == 0 &&
        v4.status == ProbeStatus::kSuccess) {
        v6.status = ProbeStatus::kSkipped;
    }
}
