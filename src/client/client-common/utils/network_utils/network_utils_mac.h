#pragma once

#include <QString>
#include <QList>
#include "types/networkinterface.h"

namespace NetworkUtils_mac
{
    // Networking
    QString ipAddressByInterfaceName(const QString &interfaceName);

    // Resolves the IPv4 address and prefix length of a specific interface by name. outIp is left
    // empty and outPrefix is left at 0 if the interface has no IPv4 or no longer exists.
    void interfaceAddressByName(const QString &interfaceName, QString &outIp, int &outPrefix);

    // IPv6 sibling of ipAddressByInterfaceName: returns the first non-link-local, non-loopback,
    // non-multicast global / ULA v6 address on the interface, or an empty string if the interface
    // has no such address. Link-local (fe80::/10) is intentionally skipped — callers want the
    // routable address the engine advertises to the helper.
    QString ipAddressByInterfaceNameV6(const QString &interfaceName);

    // IPv6 sibling of interfaceAddressByName. Same skip rules as ipAddressByInterfaceNameV6.
    // outPrefix is the number of set bits in the netmask (0..128), or 0 if no address found.
    void interfaceAddressByNameV6(const QString &interfaceName, QString &outIp, int &outPrefix);

    QString macAddressFromInterfaceName(const QString &interfaceName);
    void getDefaultRoute(QString &outGatewayIp, QString &outInterfaceName);

    // IPv6 sibling of getDefaultRoute: parses `netstat -nr -f inet6` for the first `default` row
    // with a non-empty gateway. The scope-id suffix (e.g. `fe80::1%en0` → `fe80::1`) is stripped so
    // the result parses cleanly through types::IpAddress (the interface name is reported separately
    // via outInterfaceName, so no info is lost). Both out-params are left empty when IPv6 is
    // disabled on the host or no v6 default route exists.
    void getDefaultRouteV6(QString &outGatewayIp, QString &outInterfaceName);

    QString lastConnectedNetworkInterfaceName();

    QString trueMacAddress(const QString &interfaceName);

    bool isWifiAdapter(const QString &networkInterface);
    bool isAdapterUp(const QString &networkInterfaceName);

    bool isInterfaceSpoofed(const QString &interfaceName);
    bool checkMacAddr(const QString& interfaceName, const QString& macAddr);

    bool pingWithMtu(const QString &url, int mtu);
    QString getLocalIP();
    QString getRoutingTable();

    // read DNS-servers for device name (now used for ipsec adapters for ikev2)
    // implemented with "scutil --dns" command
    // TODO: do not implement via an external util, as the output format of util may change in OS
    QStringList getDnsServersForInterface(const QString &interfaceName);

    QStringList getListOfDnsNetworkServiceEntries();
    QStringList getP2P_AWDL_NetworkInterfaces();

    // Location Services (for SSID)
    QString getWifiSsid(const QString &interface);
    bool isLocationPermissionGranted();
    bool isLocationServicesOn();
}
