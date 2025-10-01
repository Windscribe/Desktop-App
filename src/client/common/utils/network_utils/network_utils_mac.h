#pragma once

#include <QString>
#include <QList>
#include "types/networkinterface.h"

namespace NetworkUtils_mac
{
    // Networking
    QString ipAddressByInterfaceName(const QString &interfaceName);
    QString macAddressFromInterfaceName(const QString &interfaceName);
    void getDefaultRoute(QString &outGatewayIp, QString &outInterfaceName);
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
