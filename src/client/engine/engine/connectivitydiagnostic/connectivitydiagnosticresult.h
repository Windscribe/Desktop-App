#pragma once

#include <QString>
#include <QStringList>
#include <optional>

// POD result of a kNoApiConnectivity diagnostic run. Populated by the platform
// collector (local-state snapshot, section A) and by the orchestrator's async
// probes (sections B/C). The formatting helpers below produce the per-probe and
// final classification log lines (section E) so that the win/mac/linux paths all
// emit a comparable, privacy-safe report.
//
// Privacy: this struct must only ever carry result codes, timings, address-family
// counts and boolean state bits. Never store hosts, ports, credentials, resolved
// addresses, usernames, request/response bodies or private headers.

// Outcome of a single diagnostic probe. kNotRun distinguishes "this probe was
// never executed" from a definite success/failure. kSkipped marks a probe that
// was deliberately not attempted (e.g. forced IPv6 with no AAAA address), which
// must not be read as a failure. kTimeout marks a probe that WAS started but had
// not returned by the overall diagnostic cap (e.g. a stuck system getaddrinfo()),
// or one that was declined because too many resolver workers are already stalled;
// it is reported honestly instead of being hidden as kNotRun.
enum class ProbeStatus
{
    kNotRun = 0,
    kSuccess,
    kFailure,
    kSkipped,
    kTimeout,
};

// Result of a DNS probe (section B). Records only codes/timings/counts; the
// resolved addresses themselves are never stored or logged.
struct DnsProbeResult
{
    ProbeStatus status = ProbeStatus::kNotRun;
    int errorCode = 0;       // resolver / getaddrinfo error code (0 == none)
    qint64 elapsedMs = 0;
    int addressCount = 0;    // number of addresses returned (count only)
};

// Result of an app-path HTTPS probe (section C). Records result/timing, the HTTP
// status or transport error, the cert-verified bit and bytes received. errorText
// is the wsnet WSNetRequestError::toString() value only — never a response body.
struct HttpProbeResult
{
    ProbeStatus status = ProbeStatus::kNotRun;
    qint64 elapsedMs = 0;
    int httpResponseCode = 0;       // 0 if no HTTP status was produced
    bool isDnsError = false;
    bool isCurlError = false;
    bool isNoNetworkError = false;
    QString errorText;              // WSNetRequestError::toString(), redacted of bodies
    bool certVerified = false;      // request succeeded with SSL verification enabled
    qint64 bytesReceived = 0;
};

// Proxy configuration mode (redacted). Carries presence/source only, never the
// host, port or credentials.
enum class ProxyMode
{
    kUnknown = 0,
    kNone,
    kApp,       // app-configured proxy (ProxyServerController)
    kSystem,    // system / PAC proxy detected
};

struct ConnectivityDiagnosticResult
{
    // --- A. Local-state snapshot (filled by the platform collector) ---

    // Online predicates, kept separate. nullopt == unknown, distinct from a
    // definite false.
    std::optional<bool> haveActiveInterface;
    std::optional<bool> haveInternetConnectivity;

    // Default route presence, per family. nullopt == not determined.
    std::optional<bool> haveDefaultRouteV4;
    std::optional<bool> haveDefaultRouteV6;

    // Selected interface: media type/index and address-family presence. The
    // adapter name is deliberately NOT stored or logged (privacy: it can carry
    // a user-assigned, identifying label).
    QString selectedInterfaceType;
    int selectedInterfaceIndex = -1;
    std::optional<bool> selectedInterfaceHasIpv4;
    std::optional<bool> selectedInterfaceHasGlobalIpv6;

    // Proxy mode (redacted) and Windscribe firewall state (our WFP sublayer only).
    ProxyMode proxyMode = ProxyMode::kUnknown;
    std::optional<bool> windscribeFirewallOn;

    // --- B. DNS probes ---
    DnsProbeResult appDnsV4;
    DnsProbeResult appDnsV6;
    DnsProbeResult systemDnsV4;
    DnsProbeResult systemDnsV6;

    // --- C. App-path HTTPS probes ---
    HttpProbeResult appApiNormal;   // default address selection
    HttpProbeResult appApiV4;       // forced IPv4 (setOverrideIp(<A>))
    HttpProbeResult appApiV6;       // forced IPv6 (setOverrideIp(<AAAA>))
    HttpProbeResult nonApiHost;     // non-API Windscribe host, same app path

    // Overall cert-verification outcome (succeeded with SSL verification on).
    // nullopt == not determined.
    std::optional<bool> certOk;

    // --- E. Formatting helpers (centralized so all platforms match) ---

    // Final, compact one-line classification per the plan format:
    //   app_api norm/v4/v6=<r>/<r>/<r> ext v4/v6=<r>/<r> sysdns v4/v6=<r>/<r>
    //   appdns v4/v6=<r>/<r> online=<bits> route v4/v6=<y/n>/<y/n>
    //   proxy=<mode> ws_firewall=<on/off> cert_ok=<y/n>
    // "norm" is the default-address-selection API HTTPS probe; it is the only HTTP
    // probe that always runs (the forced-IP v4/v6 probes are skipped when their DNS
    // family did not resolve), so it must be in the compact line and not only in the
    // per-probe lines. The "ext" slots are emitted as n/a (external-process probes
    // are Phase 2 and out of scope here).
    QString toClassificationLine() const;

    // Expanded, human-readable dump of the section-A local snapshot (selected
    // interface, default routes, online predicates, proxy mode, firewall state).
    // Centralized here so every platform emits the same detailed line.
    QString formatLocalSnapshot() const;

    // Short, human-readable hypotheses about the likely cause of the failure,
    // derived from the collected state. Ordered from the lowest network layer
    // (interface/route) up to the highest (DNS -> HTTP -> TLS) so the log shows
    // where the chain breaks first. Privacy-safe: reads only codes/flags.
    QStringList analyzeHints() const;

    // Per-probe log lines, used by the orchestrator before the classification
    // line. The label identifies the probe, e.g. "appdns_v4" or "app_api_v6".
    static QString formatDnsProbe(const QString &label, const DnsProbeResult &r);
    static QString formatHttpProbe(const QString &label, const HttpProbeResult &r);
};

namespace connectivity_diagnostic_detail {

inline const char *probeToken(ProbeStatus s)
{
    switch (s) {
    case ProbeStatus::kSuccess: return "ok";
    case ProbeStatus::kFailure: return "fail";
    case ProbeStatus::kSkipped: return "skip";
    case ProbeStatus::kTimeout: return "t/o";
    case ProbeStatus::kNotRun:  return "n/a";
    }
    return "n/a";
}

inline const char *yesNoUnknown(const std::optional<bool> &v)
{
    if (!v.has_value())
        return "?";
    return *v ? "y" : "n";
}

inline const char *onOffUnknown(const std::optional<bool> &v)
{
    if (!v.has_value())
        return "?";
    return *v ? "on" : "off";
}

inline const char *proxyToken(ProxyMode m)
{
    switch (m) {
    case ProxyMode::kNone:    return "none";
    case ProxyMode::kApp:     return "app";
    case ProxyMode::kSystem:  return "system";
    case ProxyMode::kUnknown: return "?";
    }
    return "?";
}

} // namespace connectivity_diagnostic_detail

inline QString ConnectivityDiagnosticResult::toClassificationLine() const
{
    using namespace connectivity_diagnostic_detail;

    // "online" bits: active-interface predicate and NLM internet-connectivity
    // predicate, kept separate so a "false online" (media up, no internet) is
    // visible.
    const QString onlineBits = QString::fromLatin1("act:%1,inet:%2")
                                   .arg(QLatin1String(yesNoUnknown(haveActiveInterface)),
                                        QLatin1String(yesNoUnknown(haveInternetConnectivity)));

    // %1-%9 go in the first arg() (the multi-arg overload caps at 9 substitutions),
    // %10-%13 in the second. "norm" (appApiNormal) leads the app_api group because
    // it is the always-run probe; the forced-IP v4/v6 slots may read "skip".
    return QString::fromLatin1(
               "app_api norm/v4/v6=%1/%2/%3 ext v4/v6=n/a/n/a sysdns v4/v6=%4/%5 "
               "appdns v4/v6=%6/%7 online=%8 route v4/v6=%9/%10 proxy=%11 "
               "ws_firewall=%12 cert_ok=%13")
        .arg(QLatin1String(probeToken(appApiNormal.status)),
             QLatin1String(probeToken(appApiV4.status)),
             QLatin1String(probeToken(appApiV6.status)),
             QLatin1String(probeToken(systemDnsV4.status)),
             QLatin1String(probeToken(systemDnsV6.status)),
             QLatin1String(probeToken(appDnsV4.status)),
             QLatin1String(probeToken(appDnsV6.status)),
             onlineBits,
             QLatin1String(yesNoUnknown(haveDefaultRouteV4)))
        .arg(QLatin1String(yesNoUnknown(haveDefaultRouteV6)),
             QLatin1String(proxyToken(proxyMode)),
             QLatin1String(onOffUnknown(windscribeFirewallOn)),
             QLatin1String(yesNoUnknown(certOk)));
}

inline QString ConnectivityDiagnosticResult::formatDnsProbe(const QString &label, const DnsProbeResult &r)
{
    using namespace connectivity_diagnostic_detail;
    return QString::fromLatin1("dns %1: result=%2 elapsed=%3ms count=%4 err=%5")
        .arg(label,
             QLatin1String(probeToken(r.status)),
             QString::number(r.elapsedMs),
             QString::number(r.addressCount),
             QString::number(r.errorCode));
}

inline QString ConnectivityDiagnosticResult::formatHttpProbe(const QString &label, const HttpProbeResult &r)
{
    using namespace connectivity_diagnostic_detail;
    QString errKind;
    if (r.isDnsError)
        errKind = QStringLiteral("dns");
    else if (r.isNoNetworkError)
        errKind = QStringLiteral("nonet");
    else if (r.isCurlError)
        errKind = QStringLiteral("curl");
    else
        errKind = QStringLiteral("none");

    return QString::fromLatin1(
               "http %1: result=%2 elapsed=%3ms http=%4 errkind=%5 err=%6 cert=%7 bytes=%8")
        .arg(label,
             QLatin1String(probeToken(r.status)),
             QString::number(r.elapsedMs),
             QString::number(r.httpResponseCode),
             errKind,
             r.errorText.isEmpty() ? QStringLiteral("-") : r.errorText)
        .arg(r.certVerified ? QStringLiteral("y") : QStringLiteral("n"),
             QString::number(r.bytesReceived));
}

inline QString ConnectivityDiagnosticResult::formatLocalSnapshot() const
{
    using namespace connectivity_diagnostic_detail;
    return QString::fromLatin1(
               "iface type=%1 idx=%2 has_v4=%3 has_v6=%4 | "
               "route v4/v6=%5/%6 | online act/inet=%7/%8 | proxy=%9 ws_firewall=%10")
        .arg(selectedInterfaceType.isEmpty() ? QStringLiteral("none") : selectedInterfaceType,
             QString::number(selectedInterfaceIndex),
             QLatin1String(yesNoUnknown(selectedInterfaceHasIpv4)),
             QLatin1String(yesNoUnknown(selectedInterfaceHasGlobalIpv6)),
             QLatin1String(yesNoUnknown(haveDefaultRouteV4)),
             QLatin1String(yesNoUnknown(haveDefaultRouteV6)),
             QLatin1String(yesNoUnknown(haveActiveInterface)),
             QLatin1String(yesNoUnknown(haveInternetConnectivity)))
        .arg(QLatin1String(proxyToken(proxyMode)),
             QLatin1String(onOffUnknown(windscribeFirewallOn)));
}

inline QStringList ConnectivityDiagnosticResult::analyzeHints() const
{
    QStringList hints;

    // A timed-out probe never completed within the cap; for cause reasoning it is
    // treated as a failure (it definitely did not succeed).
    const auto isFail = [](ProbeStatus s) { return s == ProbeStatus::kFailure || s == ProbeStatus::kTimeout; };
    const auto isOk = [](ProbeStatus s) { return s == ProbeStatus::kSuccess; };
    const auto isTimeout = [](ProbeStatus s) { return s == ProbeStatus::kTimeout; };
    const auto isFalse = [](const std::optional<bool> &v) { return v.has_value() && !*v; };

    const bool anyDnsOk = isOk(appDnsV4.status) || isOk(appDnsV6.status) ||
                          isOk(systemDnsV4.status) || isOk(systemDnsV6.status);
    const bool anyDnsFail = isFail(appDnsV4.status) || isFail(appDnsV6.status) ||
                            isFail(systemDnsV4.status) || isFail(systemDnsV6.status);
    // App DNS is only "blocked" if it fails on a family where the system resolver
    // of the SAME family succeeds. A v6 failure that also fails on the system
    // resolver just means the host/network has no IPv6 — not an app problem.
    const bool appDnsBlocked = (isFail(appDnsV4.status) && isOk(systemDnsV4.status)) ||
                               (isFail(appDnsV6.status) && isOk(systemDnsV6.status));
    const bool anyHttpOk = isOk(appApiNormal.status) || isOk(appApiV4.status) ||
                           isOk(appApiV6.status);
    const bool allHttpFail = isFail(appApiNormal.status) && isFail(appApiV4.status);

    // Lowest layer first: interface / routing.
    if (isFalse(haveActiveInterface))
        hints << QStringLiteral("HINT: no active network interface - device appears offline.");
    else if (isFalse(haveInternetConnectivity))
        hints << QStringLiteral("HINT: interface is up but OS reports no internet - likely captive portal or upstream outage.");

    if (isFalse(haveDefaultRouteV4) && isFalse(haveDefaultRouteV6))
        hints << QStringLiteral("HINT: no default route on any family - no gateway or VPN left routes broken.");

    // DNS layer.
    if (!anyDnsOk && anyDnsFail)
        hints << QStringLiteral("HINT: DNS resolution failing on all paths - DNS servers unreachable or blocked.");
    else if (appDnsBlocked)
        hints << QStringLiteral("HINT: app DNS (wsnet) fails while system DNS works - firewall/proxy blocking the app or a DoH issue.");

    // A stalled system getaddrinfo() (timed out at the cap, or declined because
    // too many resolver workers are already stuck) is itself a strong signal.
    if (isTimeout(systemDnsV4.status) || isTimeout(systemDnsV6.status))
        hints << QStringLiteral("HINT: system DNS (getaddrinfo) did not return within the cap - OS resolver stalled (captive portal, broken VPN DNS, or unreachable resolver).");

    // HTTP / TLS layer (connectivity above DNS).
    if (anyDnsOk && allHttpFail)
        hints << QStringLiteral("HINT: DNS resolves but HTTPS to API fails - firewall/DPI/proxy blocking or API outage.");

    if (isFail(appApiNormal.status) && isOk(appApiV4.status))
        hints << QStringLiteral("HINT: default selection fails but forced IPv4 works - broken IPv6 connectivity.");

    if (isFail(appApiV6.status) && isOk(appApiV4.status) && haveDefaultRouteV6.value_or(false))
        hints << QStringLiteral("HINT: IPv6 route present but IPv6 probes fail - half-broken IPv6.");

    if (certOk.has_value() && !*certOk && anyHttpOk)
        hints << QStringLiteral("HINT: TLS cert verification fails though connection reached - MITM proxy/DPI or wrong system clock.");

    if (allHttpFail && isOk(nonApiHost.status))
        hints << QStringLiteral("HINT: non-API host reachable but API host not - API endpoint specifically blocked or down.");

    // Contributing factors.
    if (proxyMode == ProxyMode::kApp || proxyMode == ProxyMode::kSystem)
        hints << QStringLiteral("HINT: a proxy is configured - a misconfigured proxy can block API traffic.");

    if (windscribeFirewallOn.value_or(false) && allHttpFail)
        hints << QStringLiteral("HINT: Windscribe firewall is ON - stale WFP rules could be blocking traffic.");

    if (hints.isEmpty())
        hints << QStringLiteral("HINT: no obvious local cause; failure likely upstream (API/server side) or transient.");

    return hints;
}
