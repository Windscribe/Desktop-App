#include "connectivitydiagnostic.h"

#include <QTimer>
#include <QUrl>

#include "dnsprobes.h"
#include "httpprobes.h"
#include "iconnectivitydiagnosticcollector.h"
#include "utils/hardcodedsettings.h"
#include "utils/log/categories.h"

namespace {

// Overall wall-clock cap for a single diagnostic run. Kept above the per-probe
// HTTP timeout (5s) so a normally-slow probe completes on its own, while a hung
// system-DNS lookup or stalled transport cannot keep the run open indefinitely.
constexpr int kOverallCapMs = 8000;

// Global cooldown between runs. The diagnostic fires on login failure, which can
// repeat through API failover; this caps it to at most one run per window.
constexpr int kCooldownMs = 60000;

// Safe, unauthenticated API URL for the app HTTPS probes. Any HTTP status
// (including 401/403) counts as connectivity success; this must never be an
// auth/login endpoint.
const QString kApiProbeUrl = QStringLiteral("https://api.windscribe.com/");

} // namespace

ConnectivityDiagnostic::ConnectivityDiagnostic(QObject *parent,
                                               IConnectivityDiagnosticCollector *collector,
                                               INetworkDetectionManager *networkDetectionManager,
                                               FirewallController *firewallController)
    : QObject(parent), collector_(collector), networkDetectionManager_(networkDetectionManager),
      firewallController_(firewallController)
{
}

ConnectivityDiagnostic::~ConnectivityDiagnostic()
{
    // The probe objects are parented to this and cancel their in-flight work in
    // their own destructors, so no explicit teardown is required here.
}

void ConnectivityDiagnostic::run()
{
    // Rate limit: reject a re-entrant run, or one that arrives within the cooldown
    // window of the previous run's start.
    if (running_) {
        qCInfo(LOG_BASIC) << "ConnectivityDiagnostic: skipping, a run is already in progress";
        return;
    }
    if (cooldownTimer_.isValid() && cooldownTimer_.elapsed() < kCooldownMs) {
        qCInfo(LOG_BASIC) << "ConnectivityDiagnostic: skipping, within cooldown window";
        return;
    }

    running_ = true;
    httpProbesDone_ = false;
    cooldownTimer_.restart();
    result_ = ConnectivityDiagnosticResult();

    qCInfo(LOG_BASIC) << "ConnectivityDiagnostic: starting kNoApiConnectivity diagnostic";

    // A. Local-state snapshot — synchronous, no network I/O.
    if (collector_ != nullptr)
        collector_->collectLocalSnapshot(result_);

    // Overall cap: finishes the run with whatever the probes have reached if they
    // have not all reported by the deadline.
    overallTimer_ = new QTimer(this);
    overallTimer_->setSingleShot(true);
    connect(overallTimer_, &QTimer::timeout, this, &ConnectivityDiagnostic::onOverallTimeout);
    overallTimer_->start(kOverallCapMs);

    // B. DNS probes. As soon as the app-DNS probes report (appDnsFinished), their
    // resolved addresses drive the section-C HTTP probes (started in
    // onAppDnsReady()). The system-DNS probes are best-effort comparison data and
    // never gate the HTTP probes; their full completion (finished) only lets the
    // run end early via onDnsProbesFinished().
    const QString apiHost = QUrl(kApiProbeUrl).host();
    dnsProbes_ = new DnsProbes(this);
    connect(dnsProbes_, &DnsProbes::appDnsFinished, this, &ConnectivityDiagnostic::onAppDnsReady);
    connect(dnsProbes_, &DnsProbes::finished, this, &ConnectivityDiagnostic::onDnsProbesFinished);
    dnsProbes_->start(apiHost);
}

void ConnectivityDiagnostic::onAppDnsReady()
{
    if (!running_)
        return;

    // C. App-path HTTPS probes. Pin the forced-IP probes to the first resolved
    // A/AAAA address from the app DNS probes; an empty address skips that probe.
    // Started here, off the app-DNS-ready signal, so a hung system getaddrinfo()
    // can never hold these (highest-signal) probes back.
    const QString forcedV4 = dnsProbes_->resolvedV4().isEmpty() ? QString() : dnsProbes_->resolvedV4().first();
    const QString forcedV6 = dnsProbes_->resolvedV6().isEmpty() ? QString() : dnsProbes_->resolvedV6().first();
    const QString nonApiUrl = QStringLiteral("https://") + HardcodedSettings::instance().windscribeServerUrl();

    httpProbes_ = new HttpProbes(this);
    connect(httpProbes_, &HttpProbes::finished, this, &ConnectivityDiagnostic::onHttpProbesFinished);
    httpProbes_->start(kApiProbeUrl, nonApiUrl, forcedV4, forcedV6);
}

void ConnectivityDiagnostic::onDnsProbesFinished()
{
    if (!running_)
        return;

    maybeFinish();
}

void ConnectivityDiagnostic::onHttpProbesFinished()
{
    if (!running_)
        return;

    httpProbesDone_ = true;
    maybeFinish();
}

void ConnectivityDiagnostic::maybeFinish()
{
    // Only end the run early once the HTTP probes are done AND every DNS probe has
    // reported, so the system-DNS comparison data is captured when it is healthy.
    // A hung system-DNS lookup leaves DnsProbes::isFinished() false, so the run
    // instead ends on the overall cap — by which point the HTTP evidence has
    // already been collected.
    if (httpProbesDone_ && dnsProbes_ != nullptr && dnsProbes_->isFinished())
        finish();
}

void ConnectivityDiagnostic::onOverallTimeout()
{
    if (!running_)
        return;

    qCWarning(LOG_BASIC) << "ConnectivityDiagnostic: overall time cap reached, reporting partial results";

    // Probes still in flight at the cap (typically a stuck system getaddrinfo())
    // are reported as timed out rather than "n/a", so the log distinguishes a
    // stall from a probe that never ran. finish() then reads these results.
    if (dnsProbes_ != nullptr)
        dnsProbes_->markUnfinishedAsTimedOut();
    if (httpProbes_ != nullptr)
        httpProbes_->markUnfinishedAsTimedOut();

    finish();
}

void ConnectivityDiagnostic::copyDnsResults()
{
    if (dnsProbes_ == nullptr)
        return;
    result_.appDnsV4 = dnsProbes_->appDnsV4();
    result_.appDnsV6 = dnsProbes_->appDnsV6();
    result_.systemDnsV4 = dnsProbes_->systemDnsV4();
    result_.systemDnsV6 = dnsProbes_->systemDnsV6();
}

void ConnectivityDiagnostic::copyHttpResults()
{
    if (httpProbes_ == nullptr)
        return;
    result_.appApiNormal = httpProbes_->appApiNormal();
    result_.appApiV4 = httpProbes_->appApiV4();
    result_.appApiV6 = httpProbes_->appApiV6();
    result_.nonApiHost = httpProbes_->nonApiHost();
    result_.certOk = httpProbes_->certOk();
}

void ConnectivityDiagnostic::finish()
{
    if (!running_)
        return;

    // Read whatever the probes have reached, even on the timeout path where their
    // finished() signal may not have fired (unreported probes stay kNotRun).
    copyDnsResults();
    copyHttpResults();

    logResults();

    if (overallTimer_ != nullptr) {
        overallTimer_->stop();
        overallTimer_->deleteLater();
        overallTimer_ = nullptr;
    }
    if (dnsProbes_ != nullptr) {
        dnsProbes_->deleteLater();
        dnsProbes_ = nullptr;
    }
    if (httpProbes_ != nullptr) {
        httpProbes_->deleteLater();
        httpProbes_ = nullptr;
    }

    running_ = false;
}

void ConnectivityDiagnostic::logResults() const
{
    // A. Expanded local snapshot (interface / routes / online / proxy / firewall).
    qCInfo(LOG_BASIC).noquote() << "ConnectivityDiagnostic: local"
                                << result_.formatLocalSnapshot();

    qCInfo(LOG_BASIC).noquote() << "ConnectivityDiagnostic:"
                                << ConnectivityDiagnosticResult::formatDnsProbe(QStringLiteral("appdns_v4"), result_.appDnsV4);
    qCInfo(LOG_BASIC).noquote() << "ConnectivityDiagnostic:"
                                << ConnectivityDiagnosticResult::formatDnsProbe(QStringLiteral("appdns_v6"), result_.appDnsV6);
    qCInfo(LOG_BASIC).noquote() << "ConnectivityDiagnostic:"
                                << ConnectivityDiagnosticResult::formatDnsProbe(QStringLiteral("sysdns_v4"), result_.systemDnsV4);
    qCInfo(LOG_BASIC).noquote() << "ConnectivityDiagnostic:"
                                << ConnectivityDiagnosticResult::formatDnsProbe(QStringLiteral("sysdns_v6"), result_.systemDnsV6);

    qCInfo(LOG_BASIC).noquote() << "ConnectivityDiagnostic:"
                                << ConnectivityDiagnosticResult::formatHttpProbe(QStringLiteral("app_api_normal"), result_.appApiNormal);
    qCInfo(LOG_BASIC).noquote() << "ConnectivityDiagnostic:"
                                << ConnectivityDiagnosticResult::formatHttpProbe(QStringLiteral("app_api_v4"), result_.appApiV4);
    qCInfo(LOG_BASIC).noquote() << "ConnectivityDiagnostic:"
                                << ConnectivityDiagnosticResult::formatHttpProbe(QStringLiteral("app_api_v6"), result_.appApiV6);
    qCInfo(LOG_BASIC).noquote() << "ConnectivityDiagnostic:"
                                << ConnectivityDiagnosticResult::formatHttpProbe(QStringLiteral("non_api_host"), result_.nonApiHost);

    qCInfo(LOG_BASIC).noquote() << "ConnectivityDiagnostic: result" << result_.toClassificationLine();

    // Short, human-readable hypotheses about the likely cause. Logged as warnings
    // so they stand out from the per-probe info lines.
    const QStringList hints = result_.analyzeHints();
    for (const QString &hint : hints)
        qCWarning(LOG_BASIC).noquote() << "ConnectivityDiagnostic:" << hint;
}
