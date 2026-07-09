#pragma once

#include <QString>

#include <optional>

#include "types/networkinterface.h"

namespace NetworkUtils_win
{
    bool isInterfaceSpoofed(int interfaceIndex);
    bool pingWithMtu(const QString &url, int mtu);
    QString getLocalIP();
    QString getRoutingTable();

    // Resolves the IPv4 address and prefix length of a specific interface by IfIndex. outIp is left
    // empty and outPrefix is left at 0 if the interface has no IPv4 or no longer exists.
    void getAdapterIpAndPrefix(unsigned long ifIndex, QString &outIp, int &outPrefix);

    types::NetworkInterface currentNetworkInterface();
    QVector<types::NetworkInterface> currentNetworkInterfaces(bool includeNoInterface, bool forceUpdate = false);

    types::NetworkInterface interfaceByIndex(int index, bool &success);

    QString interfaceSubkeyName(int interfaceIndex);

    std::optional<bool> haveInternetConnectivity();

    // Default-route presence per address family. Returns nullopt (unknown) if the routing
    // table could not be read, distinct from a definite false (no default route present).
    std::optional<bool> haveDefaultRouteV4();
    std::optional<bool> haveDefaultRouteV6();

    // Reports whether the interface identified by ifIndex currently has a unicast IPv4 address
    // and/or a global (non link-local, non loopback) unicast IPv6 address. Both outputs are set
    // to false if the interface has no such address or no longer exists.
    void interfaceAddressPresence(unsigned long ifIndex, bool &hasIpv4, bool &hasGlobalIpv6);

    bool isSsidAccessAvailable();

    QString currentNetworkInterfaceGuid();
    bool haveActiveInterface();
}
