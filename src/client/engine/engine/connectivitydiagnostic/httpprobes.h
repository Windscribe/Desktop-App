#pragma once

#include <QObject>
#include <QString>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "connectivitydiagnosticresult.h"

namespace wsnet {
class WSNetCancelableCallback;
class WSNetRequestError;
}

// App-path HTTPS probes (section C) of the kNoApiConnectivity diagnostic. These
// go through the real libcurl path the app uses for API traffic (wsnet's
// httpNetworkManager), so a success here proves the actual client networking
// stack can reach the server — not just that some other socket can.
//
// For a single API URL and a single non-API Windscribe host it runs:
//   - normal   : API URL, default address selection, SSL verification ON.
//   - forced v4: API URL pinned to a resolved A address (setOverrideIp).
//   - forced v6: API URL pinned to a resolved AAAA address; skipped (not failed)
//                when no AAAA address is available.
//   - non-API  : a non-API Windscribe host, same app path, SSL verification ON.
//   - cert-check: API URL with SSL verification OFF. Not surfaced as its own
//                result slot; used only to disambiguate a cert rejection from a
//                connectivity failure and to derive certOk() (see below).
//
// Connectivity success: a probe is a success whenever the transport completed
// (WSNetRequestError::isSuccess()), regardless of the HTTP status. Reaching the
// server with any HTTP response — including 401/403 from an unauthenticated
// endpoint — proves connectivity; only the HTTP code is recorded, separately.
//
// certOk():
//   - true   if the verification-ON probe succeeded (cert verified end to end).
//   - false  if that probe failed but the verification-OFF cert-check reached the
//            server (we can connect; only the certificate is being rejected).
//   - nullopt if neither reached the server (a connectivity problem, not a cert
//            problem — we cannot tell).
//
// Every request disables the DNS cache and removes its IPs from the firewall
// whitelist on finish, so a diagnostic run never pollutes the cache or the
// whitelist/firewall exceptions.
//
// Lives on the engine thread. start() is fire-once. finished() is emitted once
// every launched probe has reported; the orchestrator may instead read the
// accessors at its overall timeout, in which case unfinished probes remain
// ProbeStatus::kNotRun.
//
// Privacy: results carry only status / timing / HTTP code / error kind+string /
// cert-verified bit / byte count. URLs, addresses, bodies and headers are never
// stored or logged. The errorText is WSNetRequestError::toString() only.
//
// Lifetime: keep the instance alive until finished() or delete it via
// deleteLater(). The destructor cancels any in-flight requests, so their
// callbacks become no-ops and destruction never blocks.
class HttpProbes : public QObject
{
    Q_OBJECT
public:
    explicit HttpProbes(QObject *parent = nullptr);
    ~HttpProbes() override;

    // Kicks off the probes. Has no effect if already started.
    //   apiUrl    : the (unauthenticated) API URL targeted by the API probes.
    //   nonApiUrl : a non-API Windscribe host URL.
    //   forcedV4  : a single resolved A address to pin the forced-IPv4 probe to;
    //               empty => that probe is skipped (not failed).
    //   forcedV6  : a single resolved AAAA address to pin the forced-IPv6 probe
    //               to; empty => that probe is skipped (not failed).
    void start(const QString &apiUrl, const QString &nonApiUrl,
               const QString &forcedV4, const QString &forcedV6);

    bool isFinished() const { return finishedCount_ >= launchedCount_; }

    // Called by the orchestrator when the overall cap fires. Reports any launched
    // probe that has not yet reported as ProbeStatus::kTimeout instead of leaving
    // it kNotRun. Skipped probes (no resolved address) stay kSkipped, and slots
    // for probes that were never launched stay kNotRun.
    void markUnfinishedAsTimedOut();

    const HttpProbeResult &appApiNormal() const { return appApiNormal_; }
    const HttpProbeResult &appApiV4() const { return appApiV4_; }
    const HttpProbeResult &appApiV6() const { return appApiV6_; }
    const HttpProbeResult &nonApiHost() const { return nonApiHost_; }

    // Overall cert-verification outcome; see the class comment for semantics.
    std::optional<bool> certOk() const { return certOk_; }

signals:
    void finished();

private:
    // Identifies which result slot a finished request belongs to.
    enum class ProbeId
    {
        kNormal,
        kV4,
        kV6,
        kNonApi,
        kCertCheck,
    };

    // The scalar outcome extracted from a finished request on the wsnet thread
    // (so the WSNetRequestError shared_ptr and the response data never cross the
    // thread boundary).
    struct ProbeOutcome
    {
        bool transportSuccess = false;
        bool isDnsError = false;
        bool isCurlError = false;
        bool isNoNetworkError = false;
        int httpResponseCode = 0;
        QString errorText;
        qint64 bytesReceived = 0;
        qint64 elapsedMs = 0;
    };

    // verifySsl == false runs with ignoreSslErrors=true (the cert-check probe).
    void launchProbe(ProbeId id, const QString &url, const QString &overrideIp, bool verifySsl);
    void onProbeFinished(ProbeId id, const ProbeOutcome &outcome);

    HttpProbeResult *slotForId(ProbeId id);
    void markProbeFinished();

    HttpProbeResult appApiNormal_;
    HttpProbeResult appApiV4_;
    HttpProbeResult appApiV6_;
    HttpProbeResult nonApiHost_;

    // Raw outcome of the verification-OFF cert-check probe, used together with
    // appApiNormal_ to derive certOk_ once all probes have reported.
    bool certCheckRan_ = false;
    bool certCheckSucceeded_ = false;
    std::optional<bool> certOk_;

    std::vector<std::shared_ptr<wsnet::WSNetCancelableCallback>> cancelHandles_;

    std::uint64_t nextRequestId_ = 0;
    int launchedCount_ = 0;
    int finishedCount_ = 0;
    bool started_ = false;
};
