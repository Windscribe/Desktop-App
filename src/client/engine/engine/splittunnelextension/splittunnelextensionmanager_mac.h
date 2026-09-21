#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "systemextensions_mac.h"
#include "types/enums.h"

class SplitTunnelExtensionManager : public QObject
{
    Q_OBJECT
public:
    static SplitTunnelExtensionManager &instance();

    // String parameters are taken by value (cheap, implicitly shared) so the internal blocks capture
    // their own copies.
    void startExtension(QString primaryInterface, QString vpnInterface);
    // An explicit stop permits retrying a failed session.
    void stopExtension();
    // Call when the OS reports the extension disabled/removed in System Settings: stop the session and
    // drop the now-stale cached manager so the next start reloads a fresh configuration from preferences.
    void resetManager();
    bool isActive() const;

    void setSplitTunnelSettings(bool isActive, bool isExclude, QStringList bundleIds, QStringList ips, QStringList hostnames);

signals:
    // Session termination preserves the preference and waits for the next VPN connection to retry.
    void startFailed(SPLIT_TUNNEL_START_FAIL_REASON reason);

private:
    friend class TestSplitTunnelExtensionManager;
    using StateQuery = std::function<void(SystemExtensions_mac::StateCallback)>;
    explicit SplitTunnelExtensionManager(StateQuery query = SystemExtensions_mac::queryFreshState);
    ~SplitTunnelExtensionManager();
    SplitTunnelExtensionManager(const SplitTunnelExtensionManager&) = delete;
    SplitTunnelExtensionManager& operator=(const SplitTunnelExtensionManager&) = delete;

    void confirmSessionEnded(void *session);
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
    StateQuery queryState_;
};
