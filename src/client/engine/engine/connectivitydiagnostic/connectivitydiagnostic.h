#pragma once

#include <QElapsedTimer>
#include <QObject>

#include "connectivitydiagnosticresult.h"

class IConnectivityDiagnosticCollector;
class INetworkDetectionManager;
class FirewallController;
class DnsProbes;
class HttpProbes;
class QTimer;

// Cross-platform orchestrator for the kNoApiConnectivity diagnostic. Lives on
// the engine thread. run() is fire-and-forget: it self-rate-limits, collects a
// local snapshot synchronously, kicks off async DNS/HTTP probes, and logs the
// per-probe lines plus a final classification line to the local debug log only.
//
// Async, non-blocking, time-bounded state machine:
//   1. Rate limit: a running_ guard plus a global cooldown, so at most one run
//      per login attempt and never more than one run per cooldown window.
//   2. Collect the local section-A snapshot synchronously (fast, no I/O).
//   3. Kick off the section-B DNS probes. As soon as the app-DNS (A/AAAA) probes
//      report, reuse their resolved addresses to drive the section-C HTTP probes
//      — the (non-cancelable) system-DNS probes are NOT a gate, they are only
//      comparison data collected best-effort within the cap.
//   4. A single overall QTimer cap bounds the whole run. The run ends early once
//      the HTTP probes AND all DNS probes are done, or on the cap firing; either
//      way the result is built, the per-probe and classification lines are
//      logged, and running_ is reset.
//
// All wsnet DNS/HTTP callbacks are marshalled back onto the engine thread by the
// probe helpers, so every slot here runs on the engine thread.
//
// Privacy: writes only to the local debug log (qCInfo/qCWarning(LOG_BASIC)) and
// never uploads or includes usernames, hosts, addresses, credentials or bodies.
class ConnectivityDiagnostic : public QObject
{
    Q_OBJECT
public:
    ConnectivityDiagnostic(QObject *parent,
                           IConnectivityDiagnosticCollector *collector,
                           INetworkDetectionManager *networkDetectionManager,
                           FirewallController *firewallController);
    ~ConnectivityDiagnostic() override;

    // Fire-and-forget. Safe to call repeatedly; self-rate-limited.
    void run();

private slots:
    // App-DNS probes (A/AAAA via the wsnet resolver) have reported. Starts the
    // section-C HTTP probes pinned to the resolved A/AAAA addresses, WITHOUT
    // waiting for the system-DNS probes (a hung getaddrinfo() must not hide the
    // HTTP evidence).
    void onAppDnsReady();
    // All DNS probes (app + system) have reported before the cap. The HTTP probes
    // were already started from onAppDnsReady(); this only lets the run end early
    // once they are done too.
    void onDnsProbesFinished();
    // HTTP probes (section C) have all reported. Finishes the run once the DNS
    // probes have also all reported (otherwise the overall cap ends it).
    void onHttpProbesFinished();
    // Overall time cap fired. Finishes the run with whatever state the probes have
    // reached so far (unfinished probes remain ProbeStatus::kNotRun).
    void onOverallTimeout();

private:
    void copyDnsResults();
    void copyHttpResults();
    // Ends the run early only once both the HTTP probes are done AND all DNS
    // probes have reported. A hung system-DNS lookup keeps it from firing, so the
    // run instead ends on the overall cap — but with the HTTP evidence collected.
    void maybeFinish();
    // Idempotent: logs the per-probe and classification lines, tears down the
    // probe objects and timer, and clears running_. Safe to call from either the
    // probe-finished slots or the overall-timeout slot.
    void finish();
    void logResults() const;

    IConnectivityDiagnosticCollector *collector_;
    INetworkDetectionManager *networkDetectionManager_;
    FirewallController *firewallController_;

    DnsProbes *dnsProbes_ = nullptr;
    HttpProbes *httpProbes_ = nullptr;
    QTimer *overallTimer_ = nullptr;

    ConnectivityDiagnosticResult result_;

    // Rate limiting. running_ rejects re-entrant runs; cooldownTimer_ rejects runs
    // that arrive within the cooldown window of the previous run's start.
    bool running_ = false;
    QElapsedTimer cooldownTimer_;

    // Set once the HTTP probes have all reported; gates the early-finish path in
    // maybeFinish(). Reset at the start of every run().
    bool httpProbesDone_ = false;
};
