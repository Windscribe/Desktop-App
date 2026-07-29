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

    // Whether the OS would let a newly created VPN tunnel device carry IPv6. Consulted so an
    // "Auto" IP Stack preference does not dial a dual-stack tunnel on a machine where IPv6 has
    // been switched off system-wide. Always true on Windows and macOS — see the implementation.
    bool isSystemIpv6Enabled();
}
