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

// IPv6 sibling of getDefaultRoute: parses /proc/net/ipv6_route to find the lowest-metric ::/0
// route, then fills outAdapterIp with a global/ULA v6 address on that interface (link-local
// addresses are intentionally skipped). Point-to-point defaults (no RTF_GATEWAY on an
// IFF_POINTOPOINT link, e.g. PPP/cellular) are reported with an empty outGatewayIp but a
// populated outInterfaceName; gateway-less defaults on broadcast interfaces and reject
// (unreachable/blackhole) defaults are skipped. All out-params are left empty if no v6 default
// route exists (IPv6 disabled, no upstream router advertisement, etc.).
void getDefaultRouteV6(QString &outGatewayIp, QString &outInterfaceName, QString &outAdapterIp, bool ignoreTun = false);

bool pingWithMtu(const QString &url, int mtu);
QString getLocalIP();
QString getRoutingTable();
QList<types::NetworkInterface> currentNetworkInterfaces(bool includeNoInterface);
types::NetworkInterface networkInterfaceByName(const QString &name);

// Resolves the IPv4 address and prefix length of a specific interface by name. outIp is left empty
// and outPrefix is left at 0 if the interface has no IPv4 or no longer exists.
void getAdapterIpAndPrefix(const QString &interfaceName, QString &outIp, int &outPrefix);

} // namespace NetworkUtils_linux
