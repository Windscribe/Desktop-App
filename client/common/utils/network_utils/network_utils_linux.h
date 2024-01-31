#pragma once

#include <QList>
#include <QString>

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

void getDefaultRoute(QString &outGatewayIp, QString &outInterfaceName, QString &outAdapterIp);
QString getLocalIP();

} // namespace NetworkUtils_linux
