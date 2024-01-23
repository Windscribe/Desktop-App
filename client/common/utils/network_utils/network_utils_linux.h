#pragma once

#include <QList>
#include <QString>

namespace NetworkUtils_linux
{
    void getDefaultRoute(QString &outGatewayIp, QString &outInterfaceName, QString &outAdapterIp);
    QString getLocalIP();

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
