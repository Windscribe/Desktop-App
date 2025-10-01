#pragma once

#include <QList>
#include <QString>

#include "types/networkinterface.h"

namespace NetworkUtils_linux
{

class RoutingTableEntry
{
public:
    int metric = 0;
    int mtu = 0;
    int flags = 0;
    int netmask = 0;
    QString interface;
    QString gateway;
    QString destination;
};

void getDefaultRoute(QString &outGatewayIp, QString &outInterfaceName, QString &outAdapterIp, bool ignoreTun = false);
bool pingWithMtu(const QString &url, int mtu);
QString getLocalIP();
QString getRoutingTable();
QList<types::NetworkInterface> currentNetworkInterfaces(bool includeNoInterface);
types::NetworkInterface networkInterfaceByName(const QString &name);

} // namespace NetworkUtils_linux
