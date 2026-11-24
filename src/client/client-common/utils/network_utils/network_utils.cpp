#include "network_utils.h"

#if defined Q_OS_WIN
#include "network_utils_win.h"
#elif defined Q_OS_MACOS
#include "network_utils_mac.h"
#elif defined Q_OS_LINUX
#include "network_utils_linux.h"
#endif

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

void NetworkUtils::getInterfaceAddress(const types::NetworkInterface &iface, QHostAddress &outIp, int &outPrefixLength)
{
    outIp = QHostAddress();
    outPrefixLength = 0;

    QString ipStr;
#ifdef Q_OS_WIN
    if (iface.interfaceIndex < 0) {
        return;
    }
    NetworkUtils_win::getAdapterIpAndPrefix(static_cast<unsigned long>(iface.interfaceIndex), ipStr, outPrefixLength);
#elif defined Q_OS_MACOS
    NetworkUtils_mac::interfaceAddressByName(iface.interfaceName, ipStr, outPrefixLength);
#elif defined Q_OS_LINUX
    NetworkUtils_linux::getAdapterIpAndPrefix(iface.interfaceName, ipStr, outPrefixLength);
#endif

    if (!ipStr.isEmpty()) {
        QHostAddress addr(ipStr);
        if (!addr.isNull() && addr.protocol() == QAbstractSocket::IPv4Protocol) {
            outIp = addr;
        }
    }
    // Contract: outIp null => failure; never report a prefix without an IP.
    if (outIp.isNull()) {
        outPrefixLength = 0;
    }
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

bool NetworkUtils::isSystemIpv6Enabled()
{
#ifdef Q_OS_LINUX
    return NetworkUtils_linux::isIpv6Enabled();
#else
    // Windows: verified harmless with Tcpip6 DisabledComponents=0xFF. tunnel.dll configures a
    // family only when Windows notifies it that the family's interface appeared on the adapter, so
    // a v6-less machine skips the v6 half instead of failing, and reports the tunnel up on v4.
    // macOS: no system-wide switch exists. networksetup -setv6off is per network service and a
    // utun is not one, so `ifconfig utunN inet6 add` always succeeds.
    return true;
#endif
}
