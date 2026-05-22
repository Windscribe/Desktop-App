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

    bool isSsidAccessAvailable();

    QString currentNetworkInterfaceGuid();
    bool haveActiveInterface();
}
