#include "network_utils_linux.h"

#include <QtAlgorithms>
#include <QHostAddress>
#include <QScopeGuard>

#include <arpa/inet.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
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

static QList<RoutingTableEntry> getRoutingTable(bool includeZeroMetricEntries)
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

        if (includeZeroMetricEntries || entry.metric != 0) {
            entries.push_back(entry);
        }
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

    QList<RoutingTableEntry> entries = getRoutingTable(false);
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
        }
    }

    if (lowestMetric != INT32_MAX) {
        for (const RoutingTableEntry& entry : qAsConst(entries)) {
            if (outInterfaceName == entry.interface && !entry.gateway.isEmpty()) {
                outGatewayIp = entry.gateway;
                break;
            }
        }
    }
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

    QList<RoutingTableEntry> entries = getRoutingTable(false);
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

} // namespace NetworkUtils_linux
