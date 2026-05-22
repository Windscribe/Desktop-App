#include "network_utils_mac.h"

#import <CoreLocation/CoreLocation.h>
#import <CoreWLAN/CoreWLAN.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QRegularExpression>
#include <QtAlgorithms>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <libproc.h>
#include <net/if.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <unistd.h>

#include "network_utils.h"
#include "../macutils.h"
#include "../utils.h"

QString NetworkUtils_mac::macAddressFromInterfaceName(const QString &interfaceName)
{
    QString command = "ifconfig " + interfaceName + " | grep 'ether' | awk '{print $2}'";
    QString strIP = Utils::execCmd(command).trimmed();
    return strIP;
}

QString NetworkUtils_mac::lastConnectedNetworkInterfaceName()
{
    QString ifname("");

    struct ifaddrs * interfaces = NULL;
    struct ifaddrs * temp_addr = NULL;

    if( getifaddrs(&interfaces) == 0 )
    {
        //Loop through linked list of interfaces
        temp_addr = interfaces;
        while( temp_addr != NULL )
        {
            if( temp_addr->ifa_addr->sa_family == AF_INET )
            {
                QString tname = temp_addr->ifa_name;
                if( tname.startsWith("utun") )
                    ifname = tname;
                else if( tname.startsWith("ipsec") )
                    ifname = tname;
                else if( tname.startsWith("ppp") )
                    ifname = tname;
            }

            temp_addr = temp_addr->ifa_next;
        }

        freeifaddrs(interfaces);
    }
    return ifname;
}

QString NetworkUtils_mac::trueMacAddress(const QString &interfaceName)
{
    QString cmdGetTrueMAC = "networksetup -getmacaddress " + interfaceName + " | awk '{print $3}'";
    return Utils::execCmd(cmdGetTrueMAC).trimmed();
}

bool NetworkUtils_mac::isInterfaceSpoofed(const QString &interfaceName)
{
    return trueMacAddress(interfaceName) != macAddressFromInterfaceName(interfaceName);
}

bool NetworkUtils_mac::isWifiAdapter(const QString &networkInterface)
{
    QString command = "echo 'list' | scutil | grep State:/Network/Interface/" + networkInterface + "/AirPort";
    QString statusResult = Utils::execCmd(command).trimmed();
    return statusResult != "";
}

bool NetworkUtils_mac::isAdapterUp(const QString &networkInterfaceName)
{
    if (isWifiAdapter(networkInterfaceName))
    {
        QString command = "echo 'show State:/Network/Interface/" + networkInterfaceName + "/AirPort' | scutil | grep 'Power Status' | awk '{print $4}'";
        QString result = Utils::execCmd(command).trimmed();
        return result == "TRUE";
    }
    else
    {
        QString command = "echo 'list' | scutil | grep State:/Network/Interface/" + networkInterfaceName + "/IPv4";
        QString result = Utils::execCmd(command).trimmed();
        return result != "";
    }
}

bool NetworkUtils_mac::pingWithMtu(const QString &url, int mtu)
{
    const QString cmd = QString("ping -c 1 -W 1000 -D -s %1 %2 2> /dev/null").arg(mtu).arg(url);
    QString result = Utils::execCmd(cmd).trimmed();

    if (result.contains("icmp_seq="))
    {
        return true;
    }
    return false;
}

QString NetworkUtils_mac::getLocalIP()
{
    QString command = "ifconfig "
                      "| grep -E \"([0-9]{1,3}\\.){3}[0-9]{1,3}\" "
                      "| grep -vE \"127\\.([0-9]{1,3}\\.){2}[0-9]{1,3}\" "
                      "| awk '{ print $2 }' | cut -f2 -d: | head -n1";
    QString result = Utils::execCmd(command).trimmed();
    return result;
}


// TODO: use system APIs for this rather than relying on flaky and changeable system tools (in terms of their output format)
void NetworkUtils_mac::getDefaultRoute(QString &outGatewayIp, QString &outInterfaceName)
{
    QString command = "netstat -nr -f inet | sed '1,3 d' | awk 'NR==1 { for (i=1; i<=NF; i++) { f[$i] = i  } } NR>1 && $(f[\"Destination\"])==\"default\" { print $(f[\"Gateway\"]), $(f[\"Netif\"]) ; exit }'";
    QString strReply = Utils::execCmd(command);
    QStringList result = strReply.split(' ');
    if (result.count() == 2)
    {
        outGatewayIp = result.first().trimmed();
        outInterfaceName = result.last().trimmed();
    }
    else
    {
        outGatewayIp.clear();
        outInterfaceName.clear();
    }
}

// TODO: use sysctl(CTL_NET, PF_ROUTE, ..., AF_INET6, NET_RT_DUMP, ...) instead of shelling out to
// netstat — same caveat as the v4 sibling above. Kept text-parsing for now to mirror the existing
// v4 helper one-for-one; switching both to PF_ROUTE is a separate cleanup.
void NetworkUtils_mac::getDefaultRouteV6(QString &outGatewayIp, QString &outInterfaceName)
{
    QString command = "netstat -nr -f inet6 | sed '1,3 d' | awk 'NR==1 { for (i=1; i<=NF; i++) { f[$i] = i  } } NR>1 && $(f[\"Destination\"])==\"default\" { print $(f[\"Gateway\"]), $(f[\"Netif\"]) ; exit }'";
    QString strReply = Utils::execCmd(command);
    QStringList result = strReply.split(' ');
    if (result.count() == 2)
    {
        outGatewayIp = result.first().trimmed();
        outInterfaceName = result.last().trimmed();

        // Strip the `%<iface>` scope suffix from link-local gateways (e.g. `fe80::1%en0` →
        // `fe80::1`). The macOS netstat output bakes the scope into the address text for
        // link-local entries; without stripping it, `types::IpAddress(gateway.toStdString())`
        // returns invalid because inet_pton rejects the `%`-form. The interface name is reported
        // in the Netif column already, so dropping the scope here is lossless.
        const int scopePos = outGatewayIp.indexOf('%');
        if (scopePos >= 0) {
            outGatewayIp = outGatewayIp.left(scopePos);
        }

        // Reject `::` / empty gateways — defence in depth, parallels the Linux v6 helper. The awk
        // filter already requires a Gateway column, so this is unlikely, but cheap to check.
        if (outGatewayIp.isEmpty() || outGatewayIp == QStringLiteral("::"))
        {
            outGatewayIp.clear();
            outInterfaceName.clear();
        }
    }
    else
    {
        outGatewayIp.clear();
        outInterfaceName.clear();
    }
}

void NetworkUtils_mac::interfaceAddressByName(const QString &interfaceName, QString &outIp, int &outPrefix)
{
    outIp.clear();
    outPrefix = 0;

    if (interfaceName.isEmpty()) {
        return;
    }

    struct ifaddrs *ifaddr = NULL;
    if (getifaddrs(&ifaddr) == -1) {
        return;
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }
        if (QString::fromStdString(ifa->ifa_name) != interfaceName) {
            continue;
        }
        // Require both IFF_UP and IFF_RUNNING so we don't return a stale cached IP for an interface whose link is
        // gone (cable yanked, Wi-Fi dropped) but whose admin state is still up.
        if ((ifa->ifa_flags & (IFF_UP | IFF_RUNNING)) != (IFF_UP | IFF_RUNNING)) {
            continue;
        }

        const struct sockaddr_in *addrIn = reinterpret_cast<const struct sockaddr_in *>(ifa->ifa_addr);
        outIp = QHostAddress(ntohl(addrIn->sin_addr.s_addr)).toString();

        if (ifa->ifa_netmask != NULL) {
            const struct sockaddr_in *maskIn = reinterpret_cast<const struct sockaddr_in *>(ifa->ifa_netmask);
            outPrefix = qPopulationCount(static_cast<quint32>(maskIn->sin_addr.s_addr));
        }
        break;
    }

    freeifaddrs(ifaddr);
}

QString NetworkUtils_mac::ipAddressByInterfaceName(const QString &interfaceName)
{
    QString ip;
    int prefix = 0;
    interfaceAddressByName(interfaceName, ip, prefix);
    return ip;
}

void NetworkUtils_mac::interfaceAddressByNameV6(const QString &interfaceName, QString &outIp, int &outPrefix)
{
    outIp.clear();
    outPrefix = 0;

    if (interfaceName.isEmpty()) {
        return;
    }

    struct ifaddrs *ifaddr = NULL;
    if (getifaddrs(&ifaddr) == -1) {
        return;
    }

    const QByteArray nameBytes = interfaceName.toUtf8();
    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET6) {
            continue;
        }
        if (qstrcmp(ifa->ifa_name, nameBytes.constData()) != 0) {
            continue;
        }
        // Same staleness guard as the v4 sibling: require both IFF_UP and IFF_RUNNING.
        if ((ifa->ifa_flags & (IFF_UP | IFF_RUNNING)) != (IFF_UP | IFF_RUNNING)) {
            continue;
        }

        const struct sockaddr_in6 *addrIn = reinterpret_cast<const struct sockaddr_in6 *>(ifa->ifa_addr);
        // Skip link-local (fe80::/10), loopback (::1), multicast, and unspecified — none of these
        // is the "primary" global/ULA v6 address the engine wants to advertise to the helper.
        if (IN6_IS_ADDR_LINKLOCAL(&addrIn->sin6_addr) ||
            IN6_IS_ADDR_LOOPBACK(&addrIn->sin6_addr) ||
            IN6_IS_ADDR_MULTICAST(&addrIn->sin6_addr) ||
            IN6_IS_ADDR_UNSPECIFIED(&addrIn->sin6_addr)) {
            continue;
        }

        char buf[INET6_ADDRSTRLEN] = {0};
        if (inet_ntop(AF_INET6, &addrIn->sin6_addr, buf, sizeof(buf)) == NULL) {
            continue;
        }
        outIp = QString::fromLatin1(buf);

        if (ifa->ifa_netmask != NULL) {
            const struct sockaddr_in6 *maskIn = reinterpret_cast<const struct sockaddr_in6 *>(ifa->ifa_netmask);
            int prefix = 0;
            for (int i = 0; i < 16; ++i) {
                prefix += qPopulationCount(static_cast<quint8>(maskIn->sin6_addr.s6_addr[i]));
            }
            outPrefix = prefix;
        }
        break;
    }

    freeifaddrs(ifaddr);
}

QString NetworkUtils_mac::ipAddressByInterfaceNameV6(const QString &interfaceName)
{
    QString ip;
    int prefix = 0;
    interfaceAddressByNameV6(interfaceName, ip, prefix);
    return ip;
}


// parse lines, like this:
// resolver #1
//      nameserver[0] : 192.168.1.1
//      if_index : 8 (en0)
QStringList NetworkUtils_mac::getDnsServersForInterface(const QString &interfaceName)
{
    QString command = "scutil --dns";
    QStringList dnsList;

    FILE *file = popen(command.toStdString().c_str(), "r");
    if (file)
    {
        char szLine[4096];
        while (fgets(szLine, sizeof(szLine), file) != 0)
        {
            QString line = QString::fromStdString(szLine);
            // parse line: "nameserver[0] : 192.168.1.1"
            if (line.contains("nameserver", Qt::CaseInsensitive))
            {
                QStringList strs = line.split(':');
                if (strs.count() == 2)
                {
                    QString dnsIp = strs[1].trimmed();
                    // get next line and parse:
                    // example:
                    //      if_index : 8 (en0)
                    if (fgets(szLine, sizeof(szLine), file) != 0)
                    {
                        QString line2 = QString::fromStdString(szLine);
                        if (line2.contains("if_index", Qt::CaseInsensitive) && line2.contains(interfaceName))
                        {
                            dnsList << dnsIp;
                        }
                    }
                }
            }
        }
        pclose(file);
    }

    dnsList.removeDuplicates();
    return dnsList;
}

QStringList NetworkUtils_mac::getListOfDnsNetworkServiceEntries()
{
    QStringList result;
    QString command = "echo 'list' | scutil | grep /Network/Service | grep DNS";
    QString cmdOutput = Utils::execCmd(command).trimmed();
    // qDebug() << "Raw result: " << cmdOutput;

    QStringList lines = cmdOutput.split('\n');
    for (QString line : lines)
    {
        if (line.contains("="))
        {
            QString entry = line.mid(line.indexOf("=")+1).trimmed();
            result.append(entry);
        }
    }
    return result;
}

bool NetworkUtils_mac::checkMacAddr(const QString &interfaceName, const QString &macAddr)
{
    return macAddressFromInterfaceName(interfaceName).toUpper().remove(':') == macAddr;
}

QStringList NetworkUtils_mac::getP2P_AWDL_NetworkInterfaces()
{
    QString res = Utils::execCmd("ifconfig -l -u").trimmed();
    QStringList list = res.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    QStringList ret;
    for (const auto &s : list)
        if (s.startsWith("p2p") || s.startsWith("awdl") || s.startsWith("llw"))
            ret << s;
    return ret;
}

QString NetworkUtils_mac::getRoutingTable()
{
    return Utils::execCmd("netstat -rn -finet").trimmed();
}

QString NetworkUtils_mac::getWifiSsid(const QString &interface)
{
    CWWiFiClient *wifiClient = [CWWiFiClient sharedWiFiClient];
    if (wifiClient == NULL) {
        return "";
    }
    CWInterface *wifiInterface = [wifiClient interfaceWithName:interface.toNSString()];
    if (wifiInterface == NULL) {
        return "";
    }
    return QString::fromNSString([wifiInterface ssid]);
}

bool NetworkUtils_mac::isLocationServicesOn()
{
    return [CLLocationManager locationServicesEnabled];
}

bool NetworkUtils_mac::isLocationPermissionGranted()
{
    CLLocationManager *locationManager = [[CLLocationManager alloc] init];
    return isLocationServicesOn() && locationManager != NULL && ([locationManager authorizationStatus] == kCLAuthorizationStatusAuthorizedAlways);
}
