#pragma once

#include <QString>

#include <optional>

#include "types/ipaddress.h"
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

    // True once `address` on the interface has finished duplicate address detection. Until then the
    // address is Tentative and Windows refuses to send from it (WSAEHOSTUNREACH), even with routes in place.
    // Matched by value: any other address on the adapter (APIPA, a leftover) says nothing about the tunnel's.
    bool isIpv4AddressPreferred(unsigned long ifIndex, const types::IpAddress &address);
    // OpenVPN and AmneziaWG (wintun) bring the tunnel up while its address is still Tentative (~3s), so
    // their connectors hold connected() back until isIpv4AddressPreferred(), polling at this interval.
    // Past the cap the tunnel is reported anyway, so an address that never settles cannot stall the connect.
    constexpr int kAddressPreferredPollMs = 50;
    constexpr int kAddressPreferredMaxWaitMs = 4000;

    bool isSsidAccessAvailable();

    QString currentNetworkInterfaceGuid();

    // Returns true if the network is still in the identifying or unidentified state (NLA).
    bool isNetworkUnidentified(const QString &adapterGUID);

    // OS identity of the network the given adapter is connected to. Returns nullopt when there is no
    // answer: the query failed or the adapter has no connection in the OS network list.
    std::optional<QString> networkIdFromInterfaceGuid(const QString &adapterGuid);

    bool haveActiveInterface();
}
