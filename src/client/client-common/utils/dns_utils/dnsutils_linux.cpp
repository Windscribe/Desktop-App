#include <QFile>
#include <QMutex>
#include <QRegularExpression>
#include <QTextStream>

#include "dnsutils.h"
#include "utils/linuxutils.h"
#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"
#include "utils/utils.h"

namespace DnsUtils
{

static const QRegularExpression whitespaceRegExp("\\s+");

// The connected VPN tunnel's interface and pushed DNS servers, used to scope the tunnel out of the OS
// default DNS read. Read from the engine thread (firewall) and the GUI thread (backend), so guarded.
static QMutex g_vpnContextMutex;
static QString g_vpnInterfaceName;
static QStringList g_tunnelDnsServers;

void setVpnContext(const QString &vpnInterfaceName, const QStringList &tunnelDnsServers)
{
    QMutexLocker locker(&g_vpnContextMutex);
    g_vpnInterfaceName = vpnInterfaceName;
    g_tunnelDnsServers = tunnelDnsServers;
}

static void getVpnContext(QString &vpnInterfaceName, QStringList &tunnelDnsServers)
{
    QMutexLocker locker(&g_vpnContextMutex);
    vpnInterfaceName = g_vpnInterfaceName;
    tunnelDnsServers = g_tunnelDnsServers;
}

std::vector<std::wstring> getOSDefaultDnsServers_NMCLI(const QString &excludeInterface)
{
    std::vector<std::wstring> dnsServers;

    // Read the full "nmcli dev show" (not "| grep DNS") so we can track which device each DNS line
    // belongs to and skip our own VPN interface, whose DNS must not be treated as an OS default.
    const QString strReply = Utils::execCmd("timeout 2 nmcli dev show 2>/dev/null");
    if (strReply.isEmpty())
    {
        qCInfo(LOG_FIREWALL_CONTROLLER) << "nmcli returned no DNS servers: NetworkManager reports no DNS for its connections";
        return dnsServers;
    }

    const QStringList lines = strReply.split('\n', Qt::SkipEmptyParts);
    QString currentDevice;
    for (auto &it : lines)
    {
        const QStringList pars = it.split(whitespaceRegExp, Qt::SkipEmptyParts);
        if (pars.size() != 2)
        {
            continue;
        }
        if (pars[0].startsWith("GENERAL.DEVICE"))
        {
            currentDevice = pars[1];
        }
        else if ((pars[0].startsWith("IP4.DNS") || pars[0].startsWith("IP6.DNS"))
                 && (excludeInterface.isEmpty() || currentDevice != excludeInterface))
        {
            dnsServers.push_back(pars[1].toStdWString());
        }
    }

    return dnsServers;
}

std::vector<std::wstring> getOSDefaultDnsServers_Resolvectl(const QString &excludeInterface)
{
    std::vector<std::wstring> dnsServers;

    const QString strReply = Utils::execCmd("timeout 2 resolvectl dns 2>/dev/null");
    if (strReply.isEmpty())
    {
        qCInfo(LOG_FIREWALL_CONTROLLER) << "resolvectl returned no DNS servers: probably systemd-resolved is not running";
        return dnsServers;
    }

    const QStringList lines = strReply.split('\n', Qt::SkipEmptyParts);
    for (auto &it : lines) {
        // Each line is "<label>: <server> [<server> ...]". Split on the first colon only, since IPv6
        // addresses contain colons too.
        const int colonIdx = it.indexOf(':');
        if (colonIdx < 0) {
            continue;
        }
        const QString label = it.left(colonIdx);
        // The label is "Link N (ifname)"; skip our own VPN interface so its pushed DNS isn't read back as
        // an OS default, and keep the generic tun/utun skip for interfaces we don't have context for.
        if (label.contains("(utun") || label.contains("(tun")
            || (!excludeInterface.isEmpty() && label.contains("(" + excludeInterface + ")"))) {
            continue;
        }
        const QStringList servers = it.mid(colonIdx + 1).split(whitespaceRegExp, Qt::SkipEmptyParts);
        for (const QString &server : servers) {
            // Strip an optional "%zone-id" suffix from IPv6 link-local entries and an optional
            // "#servername" suffix from DNS-over-TLS entries.
            dnsServers.push_back(server.section('%', 0, 0).section('#', 0, 0).toStdWString());
        }
    }

    return dnsServers;
}

std::vector<std::wstring> getOSDefaultDnsServers_ResolvConf(const QStringList &excludeDnsServers)
{
    std::vector<std::wstring> dnsServers;

    QFile file("/etc/resolv.conf");
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return dnsServers;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QStringList pars = in.readLine().split(whitespaceRegExp, Qt::SkipEmptyParts);
        if (pars.size() >= 2 && pars[0] == "nameserver") {
            // Strip an optional "%zone-id" suffix from IPv6 link-local entries.
            const QString server = pars[1].section('%', 0, 0);
            // resolv.conf is a flat, non-interface-scoped file, so a connected tunnel that manages it may
            // have written its own resolver here. Exclude the tunnel's DNS by value so we don't read it
            // back as an OS default.
            if (excludeDnsServers.contains(server)) {
                continue;
            }
            dnsServers.push_back(server.toStdWString());
        }
    }

    return dnsServers;
}

std::vector<std::wstring> getOSDefaultDnsServers()
{
    QString vpnInterfaceName;
    QStringList tunnelDnsServers;
    getVpnContext(vpnInterfaceName, tunnelDnsServers);

    // Prefer resolvectl, fall back to nmcli, then to /etc/resolv.conf, advancing whenever a source yields
    // nothing. Selection is by result, not by which binaries are installed: resolvectl ships with systemd,
    // so it is present even on hosts where systemd-resolved is not running and nmcli holds the answer.
    // nmcli is only consulted when NetworkManager is actually running; otherwise the call is pointless and
    // would block on D-Bus, so we skip straight to /etc/resolv.conf.
    // resolvectl/nmcli report DNS per interface, so the connected tunnel is scoped out by interface name;
    // resolv.conf is flat, so the tunnel's own resolvers are scoped out by value instead.
    std::vector<std::wstring> dnsServers = getOSDefaultDnsServers_Resolvectl(vpnInterfaceName);
    if (dnsServers.empty() && LinuxUtils::isNetworkManagerActive()) {
        dnsServers = getOSDefaultDnsServers_NMCLI(vpnInterfaceName);
    }
    if (dnsServers.empty()) {
        dnsServers = getOSDefaultDnsServers_ResolvConf(tunnelDnsServers);
    }
    if (dnsServers.empty()) {
        qCWarning(LOG_FIREWALL_CONTROLLER) << "Can't get OS default DNS list: resolvectl, nmcli, and /etc/resolv.conf all returned nothing";
    }

    // Drop IPv6: the Linux firewall's allowlist emits IPv4-only rules (ip .../32), so a v6 resolver would
    // produce a malformed nftables rule and fail the whole apply.
    std::vector<std::wstring> ipv4Only;
    ipv4Only.reserve(dnsServers.size());
    for (const auto &dns : dnsServers) {
        if (NetworkingValidation::isIpv4(QString::fromStdWString(dns))) {
            ipv4Only.push_back(dns);
        }
    }
    return ipv4Only;
}

}
