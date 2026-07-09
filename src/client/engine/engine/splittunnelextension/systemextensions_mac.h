#pragma once

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
    // awaiting approval), an activation request is submitted to install or (re)prompt for it.
    void checkSystemExtensionState(bool installIfMissing);

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
    // approval as needed.  Main queue only (its callback chain touches the main-queue-owned fields
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

    static SystemExtensions_mac* instance_;
};
