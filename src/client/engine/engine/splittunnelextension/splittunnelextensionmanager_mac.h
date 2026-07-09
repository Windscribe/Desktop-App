#pragma once

#include <atomic>
#include <memory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

class SplitTunnelExtensionManager : public QObject
{
    Q_OBJECT
public:
    static SplitTunnelExtensionManager &instance();

    // String parameters are taken by value (cheap, implicitly shared) so the internal blocks capture
    // their own copies.
    void startExtension(QString primaryInterface, QString vpnInterface);
    void stopExtension();
    // Call when the OS reports the extension disabled/removed in System Settings: stop the session and
    // drop the now-stale cached manager so the next start reloads a fresh configuration from preferences.
    void resetManager();
    bool isActive() const;

    void setSplitTunnelSettings(bool isActive, bool isExclude, QStringList bundleIds, QStringList ips, QStringList hostnames);

signals:
    // Emitted (from the macOS main thread) when an attempted start is given up internally -- the start
    // API failed, or a watchdog-confirmed dead session -- while the OS extension itself is present and
    // active.  The engine connects this with a queued connection and surfaces it via
    // splitTunnelingStartFailed, which alerts the user and disables the feature in the UI.
    void startFailed();

private:
    SplitTunnelExtensionManager();
    ~SplitTunnelExtensionManager();
    SplitTunnelExtensionManager(const SplitTunnelExtensionManager&) = delete;
    SplitTunnelExtensionManager& operator=(const SplitTunnelExtensionManager&) = delete;

    void reconcile();
    void setupManager();
    void dropManager();
    bool sendSettingsUpdate();

    // Read by the engine thread to decide whether to react to connection/state changes; all other
    // state lives in State and is owned by the macOS main thread (see the .mm file comment).
    std::atomic<bool> isActive_;

    // Held behind a pointer so this header stays plain C++ (State holds Objective-C objects).
    struct State;
    std::unique_ptr<State> state_;
};
