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

    types::NetworkInterface currentNetworkInterface();
    QVector<types::NetworkInterface> currentNetworkInterfaces(bool includeNoInterface, bool forceUpdate = false);

    types::NetworkInterface interfaceByIndex(int index, bool &success);

    QString interfaceSubkeyName(int interfaceIndex);

    std::optional<bool> haveInternetConnectivity();

    bool isSsidAccessAvailable();

    QString currentNetworkInterfaceGuid();
    bool haveActiveInterface();
}
