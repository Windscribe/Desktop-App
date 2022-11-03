#pragma once

#include <QList>
#include <QString>

namespace LinuxUtils
{
    QString getOsVersionString();
    void getDefaultRoute(QString &outGatewayIp, QString &outInterfaceName, QString &outAdapterIp);
    QString getLinuxKernelVersion();
    const QString getLastInstallPlatform();
    QString getLocalIP();

    // CLI
    bool isGuiAlreadyRunning();

    const QString LAST_INSTALL_PLATFORM_FILE = "/etc/windscribe/platform";
    const QString DEB_PLATFORM_NAME = QString("linux_deb_x64");
    const QString RPM_PLATFORM_NAME = QString("linux_rpm_x64");
    const QString ZST_PLATFORM_NAME = QString("linux_zst_x64");

    class RoutingTableEntry
    {
    public:
        bool isIPv4() const;

    public:
        int metric = 0;
        int hopcount = 0;
        int mtu = 0;
        int flags = 0;
        int netmask = 0; // This is the cidr-formatted mask
        int family = 0;
        QString interface;
        QString gateway;
        QString source;
        QString destination;
        QString type;
    };

    // Retrieve the routing table via Netlink API.
    QList<RoutingTableEntry> getRoutingTable(bool includeZeroMetricEntries);
}
