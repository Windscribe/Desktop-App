#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <cstdint>
#include <memory>
#include <vector>

#include "connectivitydiagnosticresult.h"

namespace wsnet {
class WSNetCancelableCallback;
class WSNetDnsRequestResult;
}

// Thread-safe back-channel between a detached system-DNS worker thread and its
// owning DnsProbes. The worker performs a single blocking getaddrinfo() lookup
// and reports its aggregate outcome (success/error code/elapsed time/address
// count — never the resolved addresses) back through this channel ONLY while
// owner is non-null. DnsProbes clears owner (under the lock) in its destructor,
// so a lookup that outlives the diagnostic — or engine shutdown — simply drops
// its result instead of touching a destroyed object or posting to a torn-down
// thread. The detached worker self-terminates once getaddrinfo() returns and
// never depends on the engine thread's event loop to clean itself up.
struct SystemDnsProbeChannel;

// Async DNS probes (section B) of the kNoApiConnectivity diagnostic. For a single
// host it runs four independent probes and records their outcome:
//   - app DNS A / AAAA via the wsnet (c-ares) resolver — the resolver the app
//     actually uses for API traffic.
//   - system DNS A / AAAA via the OS getaddrinfo(), each on a detached worker
//     thread so the engine thread is never blocked.
//
// Lives on the engine thread. start() is fire-once. appDnsFinished() is emitted
// once both app-DNS probes have reported, so the orchestrator can start the
// app-path HTTP probes without waiting on the (non-cancelable, possibly hung)
// system-DNS probes. finished() is emitted once all four probes have reported;
// the orchestrator may instead read the accessors at its overall timeout, in
// which case unfinished probes remain ProbeStatus::kNotRun.
//
// Privacy: the per-probe results carry only status / error code / elapsed time /
// address count. The app-DNS resolved addresses ARE retained (resolvedV4() /
// resolvedV6()) solely so the orchestrator can drive the forced-IP HTTP probes
// (section C); they must never be logged.
//
// Lifetime: keep the instance alive until finished() or delete it via
// deleteLater(). The destructor cancels any in-flight wsnet lookups and detaches
// the system-DNS workers (clears their back-channel owner under a lock). Those
// detached threads self-terminate when their (possibly still-running) getaddrinfo
// returns and never touch this object or the engine thread afterwards, so
// destruction never blocks and cannot leave a crash hazard behind on shutdown.
class DnsProbes : public QObject
{
    Q_OBJECT
public:
    explicit DnsProbes(QObject *parent = nullptr);
    ~DnsProbes() override;

    // Kicks off all four probes for host. Has no effect if already started.
    void start(const QString &host);

    bool isFinished() const { return appFinishedCount_ >= kAppProbeCount && systemFinishedCount_ >= kSystemProbeCount; }

    // Called by the orchestrator when the overall cap fires. Reports any probe
    // still in flight (a non-cancelable system getaddrinfo() that has not yet
    // returned, or an app lookup still pending) as ProbeStatus::kTimeout instead
    // of leaving it as kNotRun, so the log honestly shows it stalled past the cap.
    void markUnfinishedAsTimedOut();

    const DnsProbeResult &appDnsV4() const { return appDnsV4_; }
    const DnsProbeResult &appDnsV6() const { return appDnsV6_; }
    const DnsProbeResult &systemDnsV4() const { return systemDnsV4_; }
    const DnsProbeResult &systemDnsV6() const { return systemDnsV6_; }

    // App-DNS resolved addresses, retained only to drive the section-C forced-IP
    // HTTP probes. Never log these.
    const QStringList &resolvedV4() const { return resolvedV4_; }
    const QStringList &resolvedV6() const { return resolvedV6_; }

signals:
    // Emitted once both app-DNS probes (A and AAAA via the wsnet resolver) have
    // reported. The orchestrator uses this to start the app-path HTTP probes
    // WITHOUT waiting for the system-DNS probes, so a hung getaddrinfo() can no
    // longer hold back the highest-signal HTTP evidence.
    void appDnsFinished();
    // Emitted once all four probes (app + system, A + AAAA) have reported.
    void finished();

private:
    static constexpr int kAppProbeCount = 2;
    static constexpr int kSystemProbeCount = 2;

    void startAppDnsProbe(const QString &host, bool isV4);
    void startSystemDnsProbe(const QString &host, bool isV4);

    void onAppDnsFinished(bool isV4, std::shared_ptr<wsnet::WSNetDnsRequestResult> result);
    void onSystemDnsFinished(bool isV4, bool success, int errorCode, qint64 elapsedMs, int addressCount);

    void emitFinishedIfAllDone();
    // Downgrades a v6 "failure with no addresses" to kSkipped when the same
    // resolver's v4 lookup succeeded (host has no AAAA / network has no IPv6).
    static void downgradeMissingAaaaAsSkipped(DnsProbeResult &v6, const DnsProbeResult &v4);

    DnsProbeResult appDnsV4_;
    DnsProbeResult appDnsV6_;
    DnsProbeResult systemDnsV4_;
    DnsProbeResult systemDnsV6_;

    QStringList resolvedV4_;
    QStringList resolvedV6_;

    std::vector<std::shared_ptr<wsnet::WSNetCancelableCallback>> cancelHandles_;

    // Live back-channels to detached system-DNS worker threads. Retained so the
    // destructor can detach any lookup still in flight (clear its owner pointer).
    std::vector<std::shared_ptr<SystemDnsProbeChannel>> systemChannels_;

    int appFinishedCount_ = 0;
    int systemFinishedCount_ = 0;
    bool started_ = false;
};
