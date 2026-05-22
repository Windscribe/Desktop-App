#pragma once

#include <QHostAddress>
#include <QString>
#include <QVector>

#include "types/networkinterface.h"

namespace NetworkUtils
{
    QString generateRandomMacAddress();
    QString formatMacAddress(QString macAddress);
    QString normalizeMacAddress(const QString &macAddress);
    bool isValidMacAddress(const QString &macAddress);

    // Network
    QVector<types::NetworkInterface> interfacesExceptOne(const QVector<types::NetworkInterface> &interfaces, const types::NetworkInterface &exceptInterface);
    // Returns a comma-separated string of interface names, optionally including the interface index after each name.
    QString networkInterfacesToString(const QVector<types::NetworkInterface> &networkInterfaces, bool includeIndex);

    bool pingWithMtu(const QString &url, int mtu);
    QString getLocalIP();

    // Resolves the IPv4 address and prefix length of a specific interface known to the engine.
    // outIp is left null and outPrefixLength is left at 0 if the interface has no IPv4 or no
    // longer exists. Dispatches to the canonical per-platform helper.
    void getInterfaceAddress(const types::NetworkInterface &iface, QHostAddress &outIp, int &outPrefixLength);

    QString getRoutingTable();
}
