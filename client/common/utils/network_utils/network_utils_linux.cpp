#include "network_utils_linux.h"

#include <QDir>
#include <QHostAddress>
#include <QScopeGuard>
#include <QtAlgorithms>

#include <net/if.h>
#include <arpa/inet.h>
#include <linux/rtnetlink.h>
#include <linux/wireless.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../logger.h"
#include "../utils.h"

namespace NetworkUtils_linux
{

static RoutingTableEntry parseEntry(const char *buf)
{
    QStringList split = QString(buf).split("\t");
    RoutingTableEntry entry;

    if (split.size() < 9) {
        qCDebug(LOG_BASIC) << "NetworkUtils_linux::parseEntry() failed to parse routing table entry:" << buf;
        return entry;
    }

    entry.interface = split[0];
    entry.destination = QHostAddress((quint32)ntohl(split[1].toInt(nullptr, 16))).toString();
    entry.gateway = QHostAddress((quint32)ntohl(split[2].toInt(nullptr, 16))).toString();
    entry.flags = split[3].toInt();
    entry.metric = split[6].toInt();
    entry.netmask = qPopulationCount((quint32)split[7].toInt(nullptr, 16));
    entry.mtu = split[8].toInt();
    return entry;
}

static QList<RoutingTableEntry> getRoutingTableAsList()
{
    QList<RoutingTableEntry> entries;

    FILE *f = fopen("/proc/net/route", "r");
    if (f == NULL) {
        qCDebug(LOG_BASIC) << "NetworkUtils_linux::getRoutingTable() failed to open /proc/net/route:" << errno;
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
        qCDebug(LOG_BASIC) << "NetworkUtils_linux::getRoutingTable() failed to read routing table";
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

static QString getAdapterIp(QString interface)
{
    return Utils::execCmd(QString("ip -br -4 addr show %1 | grep UP | awk '{print $3}' | cut -d '/' -f 1").arg(interface)).trimmed();
}

void getDefaultRoute(QString &outGatewayIp, QString &outInterfaceName, QString &outAdapterIp, bool ignoreTun)
{
    outInterfaceName.clear();
    outGatewayIp.clear();
    outAdapterIp.clear();

    int lowestMetric = INT32_MAX;

    QList<RoutingTableEntry> entries = getRoutingTableAsList();
    for (const RoutingTableEntry& entry : qAsConst(entries)) {
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
    for (const RoutingTableEntry& entry : qAsConst(entries)) {
        QString adapterIp = getAdapterIp(entry.interface);
        if (!adapterIp.isEmpty() && entry.metric < lowestMetric) {
            lowestMetric = entry.metric;
            sLocalIP = adapterIp;
        }
    }

    if (sLocalIP.isEmpty()) {
        qCDebug(LOG_BASIC) << "LinuxUtils::getLocalIP() failed to determine the local IP";
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
    QString strReply;
    FILE *file = popen("nmcli -t -f NAME,DEVICE c show", "r");
    if (file) {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0) {
            strReply += szLine;
        }
        pclose(file);
    }

    const QStringList lines = strReply.split('\n', Qt::SkipEmptyParts);
    for (auto &it : lines) {
        const QStringList pars = it.split(':', Qt::SkipEmptyParts);
        if (pars.size() == 2) {
            if (pars[1] == ifname) {
                return pars[0];
            }
        }
    }
    return QString();
}

static void populateInterface(const QString &ifname, types::NetworkInterface &interface)
{
    interface.interfaceName = ifname;
    interface.interfaceIndex = if_nametoindex(ifname.toStdString().c_str());
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
    // Ignore loopback
    names.removeOne("lo");

    for (const QString name : names) {
        interfaces.push_back(networkInterfaceByName(name));
    }
    return interfaces;
}

types::NetworkInterface networkInterfaceByName(const QString &name)
{
    types::NetworkInterface interface = types::NetworkInterface::noNetworkInterface();
    if (!name.isEmpty()) {
        populateInterface(name, interface);
    }

    return interface;
}

} // namespace NetworkUtils_linux
