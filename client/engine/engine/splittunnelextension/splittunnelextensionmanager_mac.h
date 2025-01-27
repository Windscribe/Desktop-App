#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>

class SplitTunnelExtensionManager
{
public:
    static SplitTunnelExtensionManager &instance();

    bool startExtension(const QString &primaryInterface, const QString &vpnInterface);
    void stopExtension();
    bool isActive() const;

    void setSplitTunnelSettings(bool isActive, bool isExclude, const QStringList &bundleIds);

private:
    SplitTunnelExtensionManager();
    ~SplitTunnelExtensionManager() = default;
    SplitTunnelExtensionManager(const SplitTunnelExtensionManager&) = delete;
    SplitTunnelExtensionManager& operator=(const SplitTunnelExtensionManager&) = delete;

    bool isActive_;
    bool isExclude_;
    QStringList appPaths_;
};
