#pragma once

#include <functional>
#include <QtCore/QString>
#include <QtCore/QStringList>

class SplitTunnelExtensionManager
{
public:
    static SplitTunnelExtensionManager &instance();

    bool startExtension(const QString &primaryInterface, const QString &vpnInterface);
    void stopExtension();
    bool isActive() const;

    void setSplitTunnelSettings(bool isActive, bool isExclude, const QStringList &bundleIds, const QStringList &ips, const QStringList &hostnames);

private:
    SplitTunnelExtensionManager();
    ~SplitTunnelExtensionManager() = default;
    SplitTunnelExtensionManager(const SplitTunnelExtensionManager&) = delete;
    SplitTunnelExtensionManager& operator=(const SplitTunnelExtensionManager&) = delete;

    bool isStarted_; // Whether the extension is started
    bool isActive_; // Whether split tunnel is enabled
    bool isExclude_;
    QStringList appPaths_;
    QStringList ips_;
    QStringList hostnames_;
};
