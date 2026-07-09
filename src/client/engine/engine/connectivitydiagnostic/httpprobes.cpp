#include "httpprobes.h"

#include <QtGlobal>

#include <wsnet/WSNet.h>
#include <wsnet/WSNetCancelableCallback.h>
#include <wsnet/WSNetHttpNetworkManager.h>
#include <wsnet/WSNetHttpRequest.h>
#include <wsnet/WSNetRequestError.h>

using namespace wsnet;

namespace {

// Per-request timeout. Kept below the orchestrator's overall cap so a single slow
// probe cannot, by itself, run the whole diagnostic up to the cap.
constexpr std::uint32_t kProbeTimeoutMs = 5000;

} // namespace

HttpProbes::HttpProbes(QObject *parent) : QObject(parent)
{
}

HttpProbes::~HttpProbes()
{
    // Cancel any in-flight requests so their callbacks become no-ops. The
    // cancelable callback synchronizes with the wsnet thread, so this is safe even
    // if a callback is being invoked concurrently, and destruction never blocks.
    for (const auto &handle : cancelHandles_) {
        if (handle)
            handle->cancel();
    }
}

void HttpProbes::start(const QString &apiUrl, const QString &nonApiUrl,
                       const QString &forcedV4, const QString &forcedV6)
{
    if (started_)
        return;
    started_ = true;

    // API probe over the app's secure path (SSL verification ON), default address
    // selection.
    launchProbe(ProbeId::kNormal, apiUrl, QString(), /*verifySsl=*/true);

    // Forced-IP API probes. With no address to pin to they are skipped (not a
    // failure): the corresponding DNS family simply did not resolve.
    if (!forcedV4.isEmpty()) {
        launchProbe(ProbeId::kV4, apiUrl, forcedV4, /*verifySsl=*/true);
    } else {
        appApiV4_.status = ProbeStatus::kSkipped;
    }
    if (!forcedV6.isEmpty()) {
        launchProbe(ProbeId::kV6, apiUrl, forcedV6, /*verifySsl=*/true);
    } else {
        appApiV6_.status = ProbeStatus::kSkipped;
    }

    // Non-API Windscribe host over the same app path (SSL verification ON).
    launchProbe(ProbeId::kNonApi, nonApiUrl, QString(), /*verifySsl=*/true);

    // Cert-check: same API URL with SSL verification OFF, used only to tell a cert
    // rejection apart from a connectivity failure (drives certOk()).
    launchProbe(ProbeId::kCertCheck, apiUrl, QString(), /*verifySsl=*/false);
}

void HttpProbes::launchProbe(ProbeId id, const QString &url, const QString &overrideIp, bool verifySsl)
{
    auto httpRequest = WSNet::instance()->httpNetworkManager()->createGetRequest(
        url.toStdString(), kProbeTimeoutMs, /*isIgnoreSslErrors=*/!verifySsl);

    // Measure the live network, not a cached lookup, and never leave the probed
    // IPs behind in the firewall whitelist once the request finishes.
    httpRequest->setUseDnsCache(false);
    httpRequest->setRemoveFromWhitelistIpsAfterFinish(true);

    // Pin to a specific resolved address for the forced-IP probes.
    if (!overrideIp.isEmpty())
        httpRequest->setOverrideIp(overrideIp.toStdString());

    // Extract the scalar outcome on the wsnet thread so neither the error object
    // nor the response data ever crosses to the engine thread, then marshal the
    // small POD back onto the engine thread (pattern mirrors DnsProbes).
    auto callback = [this, id](std::uint64_t requestId, std::uint32_t elapsedMs,
                               std::shared_ptr<WSNetRequestError> error, const std::string &data) {
        Q_UNUSED(requestId);

        ProbeOutcome outcome;
        outcome.elapsedMs = static_cast<qint64>(elapsedMs);
        outcome.bytesReceived = static_cast<qint64>(data.size());
        if (error) {
            outcome.transportSuccess = error->isSuccess();
            outcome.isDnsError = error->isDnsError();
            outcome.isCurlError = error->isCurlError();
            outcome.isNoNetworkError = error->isNoNetworkError();
            outcome.httpResponseCode = error->httpResponseCode();
            outcome.errorText = QString::fromStdString(error->toString());
        }

        QMetaObject::invokeMethod(this, [this, id, outcome] {  // NOLINT: false positive for memory leak
            onProbeFinished(id, outcome);
        });
    };

    auto handle = WSNet::instance()->httpNetworkManager()->executeRequest(httpRequest, nextRequestId_++, callback);
    cancelHandles_.push_back(handle);
    ++launchedCount_;
}

HttpProbeResult *HttpProbes::slotForId(ProbeId id)
{
    switch (id) {
    case ProbeId::kNormal: return &appApiNormal_;
    case ProbeId::kV4:     return &appApiV4_;
    case ProbeId::kV6:     return &appApiV6_;
    case ProbeId::kNonApi: return &nonApiHost_;
    case ProbeId::kCertCheck: return nullptr;  // not surfaced as a result slot
    }
    return nullptr;
}

void HttpProbes::onProbeFinished(ProbeId id, const ProbeOutcome &outcome)
{
    if (id == ProbeId::kCertCheck) {
        // Verification-OFF probe: feeds certOk() only, never a result slot.
        certCheckRan_ = true;
        certCheckSucceeded_ = outcome.transportSuccess;
    } else {
        HttpProbeResult *probe = slotForId(id);
        // A completed transport means we reached the server; any HTTP status
        // (including 401/403 from an unauthenticated endpoint) counts as a
        // connectivity success. The HTTP code is recorded separately.
        probe->status = outcome.transportSuccess ? ProbeStatus::kSuccess : ProbeStatus::kFailure;
        probe->elapsedMs = outcome.elapsedMs;
        probe->httpResponseCode = outcome.httpResponseCode;
        probe->isDnsError = outcome.isDnsError;
        probe->isCurlError = outcome.isCurlError;
        probe->isNoNetworkError = outcome.isNoNetworkError;
        probe->errorText = outcome.errorText;
        probe->bytesReceived = outcome.bytesReceived;
        // These probes run with SSL verification ON, so a success means the
        // certificate verified end to end.
        probe->certVerified = outcome.transportSuccess;
    }

    // Derive the overall cert outcome from what is known so far. Recomputed on
    // every report so a read at the orchestrator's timeout reflects the latest
    // state. We only conclude "cert bad" once the verify-ON probe has actually
    // failed (not merely not-yet-run).
    if (appApiNormal_.status == ProbeStatus::kSuccess) {
        certOk_ = true;
    } else if (appApiNormal_.status == ProbeStatus::kFailure && certCheckRan_ && certCheckSucceeded_) {
        certOk_ = false;
    }

    markProbeFinished();
}

void HttpProbes::markProbeFinished()
{
    ++finishedCount_;
    if (finishedCount_ >= launchedCount_)
        emit finished();
}

void HttpProbes::markUnfinishedAsTimedOut()
{
    // A launched probe starts kNotRun and becomes kSuccess/kFailure when it
    // reports; a probe with no resolved address is set kSkipped in start(). So a
    // slot still kNotRun here was launched but had not reported by the cap.
    const auto mark = [](HttpProbeResult &p) {
        if (p.status == ProbeStatus::kNotRun)
            p.status = ProbeStatus::kTimeout;
    };
    mark(appApiNormal_);
    mark(appApiV4_);
    mark(appApiV6_);
    mark(nonApiHost_);
}
