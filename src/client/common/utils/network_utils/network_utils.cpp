#include "network_utils.h"

#include <QRegularExpression>

#include "../utils.h"

#if defined Q_OS_WIN
#include "network_utils_win.h"
#elif defined Q_OS_MACOS
#include "network_utils_mac.h"
#elif defined Q_OS_LINUX
#include "network_utils_linux.h"
#endif

QString NetworkUtils::generateRandomMacAddress()
{
    QString s;

    for (int i = 0; i < 6; i++) {
        char buf[256];
        int tp = Utils::generateIntegerRandom(0, 255);

        // Lowest bit in first byte must not be 1 ( 0 - Unicast, 1 - multicast )
        // 2nd lowest bit in first byte must be 1 ( 0 - device, 1 - locally administered mac address )
        if ( i == 0) {
            tp |= 0x02;
            tp &= 0xFE;
        }

        snprintf(buf, 256, "%s%X", tp < 16 ? "0" : "", tp);
        s += QString::fromStdString(buf);
    }
    return s;
}

QString NetworkUtils::formatMacAddress(QString macAddress)
{
    // WS_ASSERT(macAddress.length() == 12);

    QString formattedMac = QString("%1:%2:%3:%4:%5:%6")
        .arg(macAddress.mid(0,2))
        .arg(macAddress.mid(2,2))
        .arg(macAddress.mid(4,2))
        .arg(macAddress.mid(6,2))
        .arg(macAddress.mid(8,2))
        .arg(macAddress.mid(10,2));

    return formattedMac;
}

QString NetworkUtils::normalizeMacAddress(const QString &str)
{
    QString macAddr = str;
    macAddr.remove(":");
    return macAddr.toUpper();
}

bool NetworkUtils::isValidMacAddress(const QString &macAddress)
{
    // Check if the MAC address is in the format XX:XX:XX:XX:XX:XX, where X is a hex digit.
    QRegularExpression macAddressRegex1("([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}");
    // Same but no colons
    QRegularExpression macAddressRegex2("([0-9A-Fa-f]{12})");  // Fixed missing closing parenthesis
    return macAddressRegex1.match(macAddress).hasMatch() || macAddressRegex2.match(macAddress).hasMatch();
}

QVector<types::NetworkInterface> NetworkUtils::interfacesExceptOne(const QVector<types::NetworkInterface> &interfaces, const types::NetworkInterface &exceptInterface)
{
    auto differentInterfaceName = [&exceptInterface](const types::NetworkInterface &ni)
    {
        return ni.interfaceName != exceptInterface.interfaceName;
    };

    QVector<types::NetworkInterface> resultInterfaces;
    std::copy_if(interfaces.begin(), interfaces.end(),  std::back_inserter(resultInterfaces), differentInterfaceName);
    return resultInterfaces;
}

bool NetworkUtils::pingWithMtu(const QString &url, int mtu)
{
#ifdef Q_OS_WIN
    return NetworkUtils_win::pingWithMtu(url, mtu);
#elif defined Q_OS_MACOS
    return NetworkUtils_mac::pingWithMtu(url, mtu);
#elif defined Q_OS_LINUX
    return NetworkUtils_linux::pingWithMtu(url, mtu);
#endif
}

QString NetworkUtils::getLocalIP()
{
#ifdef Q_OS_WIN
    return NetworkUtils_win::getLocalIP();
#elif defined Q_OS_MACOS
    return NetworkUtils_mac::getLocalIP();
#elif defined Q_OS_LINUX
    return NetworkUtils_linux::getLocalIP();
#endif
}

QString NetworkUtils::networkInterfacesToString(const QVector<types::NetworkInterface> &networkInterfaces, bool includeIndex)
{
    QString adapters;
    for (const auto &networkInterface : networkInterfaces) {
        adapters += networkInterface.interfaceName;
        if (includeIndex) {
            adapters += QString(" (%1)").arg(networkInterface.interfaceIndex);
        }
        adapters += ", ";
    }

    if (!adapters.isEmpty()) {
        // Remove the trailing delimiter.
        adapters.chop(2);
    }

    return adapters;
}

QString NetworkUtils::getRoutingTable()
{
#ifdef Q_OS_WIN
    return NetworkUtils_win::getRoutingTable();
#elif defined Q_OS_MACOS
    return NetworkUtils_mac::getRoutingTable();
#elif defined Q_OS_LINUX
    return NetworkUtils_linux::getRoutingTable();
#endif
}
