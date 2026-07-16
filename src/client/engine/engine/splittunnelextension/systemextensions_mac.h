#pragma once

#include <atomic>

#include <QObject>

class SystemExtensions_mac : public QObject {
    Q_OBJECT
public:
    static SystemExtensions_mac* instance();

    enum SystemExtensionState {
        Active,
        Inactive,
        PendingUserApproval,
        Unknown
    };

    // The last state delivered through onExtensionStateChanged; unconfirmed queries fall back to it
    // (see unconfirmedQueryFallback in the .mm).
    SystemExtensionState lastKnownState() const { return lastKnownState_; }

    // Queries the actual state of the system extension via a properties request, then emits
    // stateChanged with the real state.  Unlike an activation request, this reports whether an
    // installed extension is currently enabled in System Settings (an extension disabled there
    // is still "activated" from an activation request's point of view, but cannot run).
    // If installIfMissing is true and the extension is not usable (not installed, disabled, or
    // awaiting approval) or is enabled at a version other than the bundled one, an activation
    // request is submitted to install, (re)prompt for, or replace it.
    void checkSystemExtensionState(bool installIfMissing);

    // The enabled installed extension's CFBundleVersion differs from the copy bundled in this app
    // (seen from the first query after an app update until the staged replacement lands).  The
    // engine must not start a session on it: the swap kills the provider's flows when it resolves.
    bool isEnabledVersionMismatch() const { return enabledVersionMismatch_; }

    // Written only by the query delegate's terminal report on the main queue (see finishWithState
    // in the .mm).
    void setEnabledVersionMismatch(bool mismatch) { enabledVersionMismatch_ = mismatch; }

signals:
    void stateChanged(SystemExtensionState newState);

public slots:
    void onExtensionStateChanged(SystemExtensionState newState);

private:
    SystemExtensions_mac();
    ~SystemExtensions_mac();

    // Delete copy constructor and assignment operator
    SystemExtensions_mac(const SystemExtensions_mac&) = delete;
    SystemExtensions_mac& operator=(const SystemExtensions_mac&) = delete;

    // Submits an activation request to install/enable the extension, (re)prompting the user for
    // approval as needed.  A same-version request is a no-op; after an app update it stages the
    // bundled extension as a replacement of the running one.  No-op while a previous activation is
    // still in flight.  Main queue only (its callback chain touches the main-queue-owned fields
    // below).
    void requestActivation();

    // The fields below are only ever touched on the main queue.
    SystemExtensionState lastKnownState_ = Unknown;

    // A properties query is outstanding; concurrent rechecks are dropped rather than stacked.
    bool isCheckingState_ = false;

    // An install request arrived while a passive query was in flight; honored when that query resolves
    // so the explicit enable is not swallowed by the dedup.
    bool pendingInstall_ = false;

    // The in-flight query is already the one retry of an unconfirmed result.
    bool queryRetried_ = false;

    // An activation request is outstanding and has not yet reported; duplicates in that window are
    // dropped.  Cleared once it reports (including PendingUserApproval), so an explicit re-enable can
    // supersede a prompt the user dismissed (see requestActivation).  Deliberately no watchdog: the OS
    // always delivers some delegate report, and a submit with no report after it is diagnosable in logs.
    bool activationInFlight_ = false;

    // Identity of the most recent activation request (unretained, compared only, never dereferenced).
    // A prompt left pending stays open at the OS after its report clears the flag; when a re-enable
    // submits the successor that supersedes it, macOS fails the old request, and that late report must
    // not do bookkeeping that now belongs to the successor.
    void* currentActivationRequest_ = nullptr;

    // Atomic, unlike the fields above: written on the main queue, read from the engine thread.
    std::atomic<bool> enabledVersionMismatch_ = false;

    static SystemExtensions_mac* instance_;
};
