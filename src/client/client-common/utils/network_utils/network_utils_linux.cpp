#include "network_utils_linux.h"

#include <QDir>
#include <QHostAddress>
#include <QScopeGuard>
#include <QtAlgorithms>

#include <net/if.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_arp.h>
#include <linux/rtnetlink.h>
#include <linux/wireless.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../linuxutils.h"
#include "../log/categories.h"
#include "../utils.h"

namespace NetworkUtils_linux
{

static RoutingTableEntry parseEntry(const char *buf)
{
    QStringList split = QString(buf).split("\t");
    RoutingTableEntry entry;

    if (split.size() < 9) {
        qCWarning(LOG_BASIC) << "NetworkUtils_linux::parseEntry() failed to parse routing table entry:" << buf;
        return entry;
    }

    entry.interface = split[0];
    entry.destination = QHostAddress((quint32)ntohl(split[1].toUInt(nullptr, 16))).toString();
    entry.gateway = QHostAddress((quint32)ntohl(split[2].toUInt(nullptr, 16))).toString();
    entry.flags = split[3].toInt();
    entry.metric = split[6].toInt();
    entry.netmask = qPopulationCount((quint32)split[7].toUInt(nullptr, 16));
    entry.mtu = split[8].toInt();
    return entry;
}

static QList<RoutingTableEntry> getRoutingTableAsList()
{
    QList<RoutingTableEntry> entries;

    FILE *f = fopen("/proc/net/route", "r");
    if (f == NULL) {
        qCCritical(LOG_BASIC) << "NetworkUtils_linux::getRoutingTable() failed to open /proc/net/route:" << errno;
        return entries;
    }

    auto exitGuard = qScopeGuard([&] {
        fclose(f);
    });

    const size_t len = 1024;
    char buf[len];
    memset(buf, 0, len);

    // skip the first line, which is just labels
    if (fgets(buf, 1024, f) == NULL) {
        qCCritical(LOG_BASIC) << "NetworkUtils_linux::getRoutingTable() failed to read routing table";
        return entries;
    }

    // parse remaining lines
    while (fgets(buf, 1024, f) != NULL) {
        RoutingTableEntry entry = parseEntry(buf);
        if (entry.interface.isEmpty()) {
            // skip invalid entries
            continue;
        }

        entries.push_back(entry);
    }

    return entries;
}

void getAdapterIpAndPrefix(const QString &interfaceName, QString &outIp, int &outPrefix)
{
    outIp.clear();
    outPrefix = 0;

    if (interfaceName.isEmpty()) {
        return;
    }

    struct ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1) {
        return;
    }

    auto guard = qScopeGuard([&] { freeifaddrs(ifaddr); });

    const QByteArray nameBytes = interfaceName.toUtf8();
    for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }
        if (qstrcmp(ifa->ifa_name, nameBytes.constData()) != 0) {
            continue;
        }
        // Require both IFF_UP and IFF_RUNNING so we don't return a stale cached IP for an interface whose link is
        // gone (cable yanked, Wi-Fi dropped) but whose admin state is still up.
        if ((ifa->ifa_flags & (IFF_UP | IFF_RUNNING)) != (IFF_UP | IFF_RUNNING)) {
            continue;
        }

        const sockaddr_in *addrIn = reinterpret_cast<const sockaddr_in *>(ifa->ifa_addr);
        outIp = QHostAddress(ntohl(addrIn->sin_addr.s_addr)).toString();

        if (ifa->ifa_netmask != nullptr) {
            const sockaddr_in *maskIn = reinterpret_cast<const sockaddr_in *>(ifa->ifa_netmask);
            outPrefix = qPopulationCount(static_cast<quint32>(maskIn->sin_addr.s_addr));
        }
        return;
    }
}

static QString getAdapterIp(QString interface)
{
    QString ip;
    int prefix = 0;
    getAdapterIpAndPrefix(interface, ip, prefix);
    return ip;
}

// Internal helper for getAdapterIpV6 (defined below). Not exported: the engine's only
// v6 consumer is the AdapterGatewayInfo population path, which only needs the address
// string — see getAdapterIpV6.
static void getAdapterIpAndPrefixV6(const QString &interfaceName, QString &outIp, int &outPrefix)
{
    outIp.clear();
    outPrefix = 0;

    if (interfaceName.isEmpty()) {
        return;
    }

    struct ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1) {
        return;
    }

    auto guard = qScopeGuard([&] { freeifaddrs(ifaddr); });

    const QByteArray nameBytes = interfaceName.toUtf8();
    for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET6) {
            continue;
        }
        if (qstrcmp(ifa->ifa_name, nameBytes.constData()) != 0) {
            continue;
        }
        if ((ifa->ifa_flags & (IFF_UP | IFF_RUNNING)) != (IFF_UP | IFF_RUNNING)) {
            continue;
        }

        const sockaddr_in6 *addrIn = reinterpret_cast<const sockaddr_in6 *>(ifa->ifa_addr);
        // Skip link-local (fe80::/10), loopback (::1), multicast, and unspecified — none of these
        // is the "primary" global/ULA v6 address the engine wants to advertise to the helper.
        if (IN6_IS_ADDR_LINKLOCAL(&addrIn->sin6_addr) ||
            IN6_IS_ADDR_LOOPBACK(&addrIn->sin6_addr) ||
            IN6_IS_ADDR_MULTICAST(&addrIn->sin6_addr) ||
            IN6_IS_ADDR_UNSPECIFIED(&addrIn->sin6_addr)) {
            continue;
        }

        char buf[INET6_ADDRSTRLEN] = {0};
        if (inet_ntop(AF_INET6, &addrIn->sin6_addr, buf, sizeof(buf)) == nullptr) {
            continue;
        }
        outIp = QString::fromLatin1(buf);

        if (ifa->ifa_netmask != nullptr) {
            const sockaddr_in6 *maskIn = reinterpret_cast<const sockaddr_in6 *>(ifa->ifa_netmask);
            int prefix = 0;
            for (int i = 0; i < 16; ++i) {
                prefix += qPopulationCount(static_cast<quint8>(maskIn->sin6_addr.s6_addr[i]));
            }
            outPrefix = prefix;
        }
        return;
    }
}

static QString getAdapterIpV6(const QString &interface)
{
    QString ip;
    int prefix = 0;
    getAdapterIpAndPrefixV6(interface, ip, prefix);
    return ip;
}

// True when the interface carries IFF_POINTOPOINT (PPP, cellular, tun). A gateway-less
// default is only usable dev-only on such links; on a broadcast interface the kernel would
// NDP every off-link destination on-link and blackhole it, so those must keep a real gateway.
static bool isPointToPoint(const QString &interface)
{
    struct ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) != 0) {
        return false;
    }
    auto freeGuard = qScopeGuard([&] { freeifaddrs(ifaddr); });

    const QByteArray nameBytes = interface.toUtf8();
    for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (qstrcmp(ifa->ifa_name, nameBytes.constData()) == 0) {
            return (ifa->ifa_flags & IFF_POINTOPOINT) != 0;
        }
    }
    return false;
}

void getDefaultRoute(QString &outGatewayIp, QString &outInterfaceName, QString &outAdapterIp, bool ignoreTun)
{
    outInterfaceName.clear();
    outGatewayIp.clear();
    outAdapterIp.clear();

    int lowestMetric = INT32_MAX;

    QList<RoutingTableEntry> entries = getRoutingTableAsList();
    for (const RoutingTableEntry& entry : std::as_const(entries)) {
        // if ignoring tun interfaces, remove them from contention
        if (ignoreTun && (entry.interface.startsWith("tun") || entry.interface.startsWith("utun"))) {
            continue;
        }
        // only consider routes which have a destination of 0.0.0.0.
        // filtering by metric alone is not enough, because when an interface first comes up, network manager will add 20000 to the metric
        // if it has not yet passed a connectivity check
        if (entry.metric < lowestMetric && entry.destination == "0.0.0.0") {
            lowestMetric = entry.metric;
            outInterfaceName = entry.interface;
            outAdapterIp = getAdapterIp(entry.interface);
            outGatewayIp = entry.gateway;
        }
    }
}

// Convert 32 hex digits (no separators) — the form used by /proc/net/ipv6_route — into a textual
// IPv6 address via inet_ntop. Returns an empty string on malformed input.
static QString hexToIpV6String(const QString &hex32)
{
    if (hex32.length() != 32) {
        return QString();
    }
    struct in6_addr addr;
    memset(&addr, 0, sizeof(addr));
    for (int i = 0; i < 16; ++i) {
        bool ok = false;
        const uint b = hex32.mid(i * 2, 2).toUInt(&ok, 16);
        if (!ok) {
            return QString();
        }
        addr.s6_addr[i] = static_cast<uint8_t>(b);
    }
    char buf[INET6_ADDRSTRLEN] = {0};
    if (inet_ntop(AF_INET6, &addr, buf, sizeof(buf)) == nullptr) {
        return QString();
    }
    return QString::fromLatin1(buf);
}

void getDefaultRouteV6(QString &outGatewayIp, QString &outInterfaceName, QString &outAdapterIp, bool ignoreTun)
{
    outInterfaceName.clear();
    outGatewayIp.clear();
    outAdapterIp.clear();

    // /proc/net/ipv6_route line layout (space-separated, 16-byte address fields are 32 hex chars
    // without colons):
    //   dst dst_prefix src src_prefix nexthop metric refcnt usecnt flags ifname
    // Flags come from include/uapi/linux/route.h (shared with v4); we care about:
    //   RTF_UP      = 0x0001
    //   RTF_GATEWAY = 0x0002
    //   RTF_REJECT  = 0x0200  (unreachable / blackhole / prohibit)
    constexpr int kRtfUp = 0x0001;
    constexpr int kRtfGateway = 0x0002;
    constexpr int kRtfReject = 0x0200;

    FILE *f = fopen("/proc/net/ipv6_route", "r");
    if (f == nullptr) {
        // IPv6 may be disabled on the host (/proc/net/ipv6_route absent). Not an error.
        return;
    }
    auto exitGuard = qScopeGuard([&] { fclose(f); });

    uint32_t lowestMetric = UINT32_MAX;
    bool lowestHasGateway = false;

    char buf[1024];
    while (fgets(buf, sizeof(buf), f) != nullptr) {
        const QStringList split = QString::fromLatin1(buf).trimmed()
                                  .split(QChar(' '), Qt::SkipEmptyParts);
        if (split.size() < 10) {
            continue;
        }

        const QString interface = split[9];
        if (interface == QStringLiteral("lo") || (ignoreTun && (interface.startsWith("tun") || interface.startsWith("utun")))) {
            continue;
        }

        bool ok = false;
        const int prefixLength = split[1].toInt(&ok, 16);
        if (!ok || prefixLength != 0) {
            continue;
        }
        const QString destination = hexToIpV6String(split[0]);
        if (destination.isEmpty() || destination != QStringLiteral("::")) {
            continue;
        }

        // Kernel writes flags and metric as %08x — full 32-bit unsigned fields. Use
        // toUInt to avoid silently rejecting values with the high bit set (toInt
        // would treat them as out-of-range for int32_t). Real-world values stay
        // well below 0x80000000, but the unsigned parse matches the source format.
        const uint32_t flags = split[8].toUInt(&ok, 16);
        // Skip reject routes (unreachable/blackhole/prohibit ::/0, e.g. NetworkManager
        // ipv6.method=disabled): they sit on lo with no gateway and are not a real egress.
        // The old RTF_GATEWAY requirement excluded them implicitly; the relaxed check below
        // must filter them explicitly.
        if (!ok || !(flags & kRtfUp) || (flags & kRtfReject)) {
            continue;
        }

        // Point-to-point defaults (PPP, cellular) have no RTF_GATEWAY and a :: nexthop; they
        // are reachable via the interface alone, so report them with an empty gateway rather
        // than discarding them. A gateway route must carry a usable nexthop, so a malformed/::
        // one is still skipped. A gateway-less default on a broadcast interface is NOT usable
        // dev-only (the kernel would NDP off-link destinations on-link and blackhole them), so
        // require IFF_POINTOPOINT before accepting it.
        QString gateway;
        if (flags & kRtfGateway) {
            gateway = hexToIpV6String(split[4]);
            if (gateway.isEmpty() || gateway == QStringLiteral("::")) {
                continue;
            }
        } else if (!isPointToPoint(interface)) {
            continue;
        }

        const uint32_t metric = split[5].toUInt(&ok, 16);
        if (!ok) {
            continue;
        }

        // Same rationale as the v4 default-route pick: pick lowest metric to avoid stale
        // not-yet-connectivity-checked routes that NetworkManager bumps by +20000. On a
        // metric tie, prefer a gatewayed default over a gateway-less one so we don't drop a
        // usable router in favour of a same-metric on-link entry.
        const bool hasGateway = !gateway.isEmpty();
        if (metric < lowestMetric || (metric == lowestMetric && hasGateway && !lowestHasGateway)) {
            lowestMetric = metric;
            lowestHasGateway = hasGateway;
            outInterfaceName = interface;
            outGatewayIp = gateway;
            outAdapterIp = getAdapterIpV6(interface);
        }
    }
}

bool pingWithMtu(const QString &url, int mtu)
{
    const QString cmd = QString("ping -4 -c 1 -W 1 -M do -s %1 %2 2> /dev/null").arg(mtu).arg(url);
    QString result = Utils::execCmd(cmd).trimmed();

    return result.contains("icmp_seq=");
}

QString getLocalIP()
{
    // Yegor and Clayton found this command to work on many distros, including old ones.
    QString sLocalIP = Utils::execCmd("hostname -I | awk '{print $1}'").trimmed();

    // Check if we received a valid IPv4 address.
    QHostAddress addr;
    if (addr.setAddress(sLocalIP) && addr.protocol() == QAbstractSocket::IPv4Protocol) {
        return sLocalIP;
    }

    sLocalIP.clear();
    int lowestMetric = INT32_MAX;

    QList<RoutingTableEntry> entries = getRoutingTableAsList();
    for (const RoutingTableEntry& entry : std::as_const(entries)) {
        QString adapterIp = getAdapterIp(entry.interface);
        if (!adapterIp.isEmpty() && entry.metric < lowestMetric) {
            lowestMetric = entry.metric;
            sLocalIP = adapterIp;
        }
    }

    if (sLocalIP.isEmpty()) {
        qCCritical(LOG_BASIC) << "LinuxUtils::getLocalIP() failed to determine the local IP";
    }

    return sLocalIP;
}

QString getRoutingTable()
{
    return Utils::execCmd("cat /proc/net/route");
}

static QString getMacAddressByIfName(const QString &ifname)
{
    QString ret;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd != -1) {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name , ifname.toStdString().c_str() , IFNAMSIZ-1);
        if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
            unsigned char *mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
            // format mac address
            ret = QString::asprintf("%.2X:%.2X:%.2X:%.2X:%.2X:%.2X" , mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }
        close(fd);
    }
    return ret;
}

static bool isActiveByIfName(const QString &ifname)
{
    bool ret = false;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd != -1) {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name , ifname.toStdString().c_str() , IFNAMSIZ-1);
        if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0) {
            ret = (ifr.ifr_flags & ( IFF_UP | IFF_RUNNING )) == ( IFF_UP | IFF_RUNNING );
        }
        close(fd);
    }
    return ret;
}

static bool checkWirelessByIfName(const QString &ifname)
{
    bool ret = false;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd != -1) {
        struct iwreq pwrq;
        memset(&pwrq, 0, sizeof(pwrq));
        strncpy(pwrq.ifr_name, ifname.toStdString().c_str(), IFNAMSIZ-1);
        if (ioctl(fd, SIOCGIWNAME, &pwrq) != -1) {
            ret = true;
        }
        close(fd);
    }
    return ret;
}

static QString getNetworkForInterface(const QString &ifname)
{
#ifndef CLI_ONLY
    if (LinuxUtils::isNetworkManagerActive()) {
        const QString strReply = Utils::execCmd("timeout 2 nmcli -t -f NAME,DEVICE c show");
        const QStringList lines = strReply.split('\n', Qt::SkipEmptyParts);
        for (auto &it : lines) {
            // nmcli -t backslash-escapes ':' and '\' in values; DEVICE contains no colons, so the
            // separator is the last colon.
            const int sep = it.lastIndexOf(':');
            if (sep >= 0 && it.mid(sep + 1) == ifname) {
                QString name = it.left(sep);
                name.replace("\\:", ":").replace("\\\\", "\\");
                return name;
            }
        }
    }
#endif

    // The network is not managed by NetworkManager (CLI-only build, the daemon is not running, or it has
    // no connection for this interface).
    // Instead, we use iw to get the SSID if it is Wi-Fi.  Otherwise, the name is just the name of the interface.
    if (!checkWirelessByIfName(ifname)) {
        return ifname;
    }

    const QString command = QString("iw dev ") + ifname + QString(" link | grep SSID | cut -d ' ' -f 2-");
    return Utils::execCmd(command).trimmed();
}

static void populateInterface(const QString &ifname, types::NetworkInterface &interface)
{
    interface.interfaceName = ifname;
    interface.interfaceIndex = if_nametoindex(ifname.toStdString().c_str());
    if (interface.interfaceIndex == 0) {
        // interface index can not be 0 on Linux, this is an error.
        interface.interfaceIndex = -1;
    }
    interface.physicalAddress = getMacAddressByIfName(ifname);
    interface.networkOrSsid = getNetworkForInterface(ifname);

    if (checkWirelessByIfName(ifname)) {
        interface.interfaceType = NETWORK_INTERFACE_WIFI;
        interface.friendlyName = "Wi-Fi";
    } else {
        interface.interfaceType = NETWORK_INTERFACE_ETH;
        interface.friendlyName = "Ethernet";
    }

    interface.active = isActiveByIfName(ifname);
}

QList<types::NetworkInterface> currentNetworkInterfaces(bool includeNoInterface)
{
    QList<types::NetworkInterface> interfaces;
    QStringList names;

    if (includeNoInterface) {
        interfaces.push_back(types::NetworkInterface::noNetworkInterface());
    }

    QDir dir("/sys/class/net");
    names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString name : names) {
        // Skip non-physical devices
        QDir deviceDir("/sys/class/net/" + name +  "/device");
        if (!deviceDir.exists()) {
            continue;
        }
        // Skip interfaces that aren't up
        if (!isActiveByIfName(name)) {
            continue;
        }
        interfaces.push_back(networkInterfaceByName(name));
    }
    return interfaces;
}

types::NetworkInterface networkInterfaceByName(const QString &name)
{
    types::NetworkInterface interface = types::NetworkInterface::noNetworkInterface();

    // Exclude loopback type
    QString type;
    QFile f("/sys/class/net/" + name + "/type");
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&f);
        type = in.readAll().trimmed();
    }

    bool ok = false;
    if (!type.isEmpty()) {
        int interfaceType = type.toInt(&ok);
        if (!ok || interfaceType == ARPHRD_LOOPBACK) {
            return interface;
        }
    }

    if (!name.isEmpty()) {
        populateInterface(name, interface);
    }

    return interface;
}

} // namespace NetworkUtils_linux
