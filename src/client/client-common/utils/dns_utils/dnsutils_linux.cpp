#include <QMutex>
#include <QProcess>

#include "dnsleakparse.h"
#include "dnsutils.h"
#include "types/ipaddress.h"
#include "utils/linuxutils.h"
#include "utils/log/categories.h"

namespace DnsUtils
{

// Returns the exit code, or -1 when the command was killed or never ran — only then is `out` a prefix cut
// mid-line and unsafe to parse. A non-zero exit with intact output is the caller's judgement, because the
// two commands differ: resolvectl prints every link it read and returns the first error, so its answer is
// still usable, while a killed nmcli leaves a truncated block. Utils::execCmd cannot express any of this,
// having dropped the status entirely.
static int runCommand(const QString &program, const QStringList &args, QString &out)
{
    QProcess p;
    p.start(program, args);
    if (!p.waitForFinished(2000)) {
        p.kill();
        p.waitForFinished(500);
        qCWarning(LOG_FIREWALL_CONTROLLER) << program << "did not finish in time; ignoring its output";
        return -1;
    }
    if (p.exitStatus() != QProcess::NormalExit) {
        qCWarning(LOG_FIREWALL_CONTROLLER) << program << "died on a signal; ignoring its output";
        return -1;
    }
    out = QString::fromLocal8Bit(p.readAllStandardOutput());
    return p.exitCode();
}

// DnsLeakParse is shared with the root helper, which logs plain strings via spdlog. The client's default
// spdlog logger interpolates its payload into a JSON object, so rejected tokens have to come back
// through our own category instead of the parser picking a backend.
static void warnToken(const std::string &tok)
{
    qCWarning(LOG_FIREWALL_CONTROLLER) << "Skipping unusable DNS server token:" << QString::fromStdString(tok);
}

// Snapshot entries are packed "address[:port]". This project's DNS API is a flat list of addresses and
// the firewall allowlist it feeds emits no port, so drop the port here — an unsplit entry would fail
// UniqueIpList's isIp() check and be discarded without a trace.
static std::vector<std::wstring> toWStringList(const std::vector<std::string> &entries)
{
    std::vector<std::wstring> out;
    out.reserve(entries.size());
    for (const auto &entry : entries) {
        std::string address;
        uint16_t port = 0;
        if (!types::splitEndpoint(entry, address, port)) {
            qCWarning(LOG_FIREWALL_CONTROLLER) << "Skipping unparseable DNS entry:" << QString::fromStdString(entry);
            continue;
        }
        out.push_back(QString::fromStdString(address).toStdWString());
    }
    return out;
}

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

// Each source below ASSIGNS v4/v6 rather than adding to them, so the chain in getOSDefaultDnsServers()
// replaces one source's answer with the next and any per-source post-processing (resolv.conf's tunnel
// exclusion) touches only what that source read. DnsLeakParse itself appends, hence the clear.
static void clearBoth(std::vector<std::string> &v4, std::vector<std::string> &v6)
{
    v4.clear();
    v6.clear();
}

static void getOSDefaultDnsServers_NMCLI(const QString &excludeInterface,
                                         std::vector<std::string> &v4, std::vector<std::string> &v6)
{
    clearBoth(v4, v6);

    // Read the full "nmcli dev show" (not "| grep DNS") so we can track which device each DNS line
    // belongs to and skip our own VPN interface, whose DNS must not be treated as an OS default.
    // Any failure discards: nmcli's per-device blocks are flushed in stdio chunks, so a partial run is a
    // truncated block, not a shorter list. The helper classifies it the same way.
    QString strReply;
    if (runCommand("nmcli", {"dev", "show"}, strReply) != 0 || strReply.isEmpty())
    {
        qCInfo(LOG_FIREWALL_CONTROLLER) << "nmcli returned no DNS servers: NetworkManager reports no DNS for its connections";
        return;
    }

    DnsLeakParse::parseNmcli(strReply.toStdString(), excludeInterface.toStdString(), v4, v6, warnToken);
}

static void getOSDefaultDnsServers_Resolvectl(const QString &excludeInterface,
                                              std::vector<std::string> &v4, std::vector<std::string> &v6)
{
    clearBoth(v4, v6);

    // Only a kill discards. resolvectl's status_all prints every link it read and returns the first
    // error, so a non-zero exit still carries a complete answer for those links — the helper reaches the
    // same conclusion, and this list feeds an allowlist where a partial answer over-blocks rather than
    // leaks, so discarding it is the worse direction.
    QString strReply;
    const int rc = runCommand("resolvectl", {"dns"}, strReply);
    if (rc < 0 || strReply.isEmpty())
    {
        qCInfo(LOG_FIREWALL_CONTROLLER) << "resolvectl returned no DNS servers: probably systemd-resolved is not running";
        return;
    }
    if (rc != 0) {
        qCWarning(LOG_FIREWALL_CONTROLLER) << "resolvectl dns exited" << rc << "- using the links it did report";
    }

    // Parsing is shared with the helper's leak-protection snapshot so a fix lands on both sides at once;
    // only policy differs. The extra "(tun"/"(utun" exemption is ours, since setVpnContext may not have
    // run yet.
    DnsLeakParse::parseResolvectl(strReply.toStdString(), excludeInterface.toStdString(), v4, v6,
                                  {"(tun", "(utun"}, warnToken);
}

static void getOSDefaultDnsServers_ResolvConf(const QStringList &excludeDnsServers,
                                              std::vector<std::string> &v4, std::vector<std::string> &v6)
{
    clearBoth(v4, v6);
    if (!DnsLeakParse::parseResolvConfFile("/etc/resolv.conf", v4, v6, warnToken)) {
        qCWarning(LOG_FIREWALL_CONTROLLER) << "/etc/resolv.conf could not be read";
    }

    // resolv.conf is flat, so a connected tunnel that manages it may have written its own resolver here.
    // removeMatching excludes the tunnel's DNS by address value, as the helper's identical strip does.
    std::vector<std::string> exclude;
    exclude.reserve(excludeDnsServers.size());
    for (const QString &s : excludeDnsServers) {
        exclude.push_back(s.toStdString());
    }
    DnsLeakParse::removeMatching(v4, exclude, warnToken);
    DnsLeakParse::removeMatching(v6, exclude, warnToken);
}

std::vector<std::wstring> getOSDefaultDnsServers()
{
    QString vpnInterfaceName;
    QStringList tunnelDnsServers;
    getVpnContext(vpnInterfaceName, tunnelDnsServers);

    // Selection is by result, not by which binaries exist: resolvectl ships with systemd even where
    // resolved is not running and nmcli holds the answer. nmcli is skipped unless NetworkManager is up,
    // since the call would otherwise block on D-Bus. The tunnel is scoped out by interface name for
    // resolvectl/nmcli and by value for flat resolv.conf.
    //
    // Only IPv4 is returned, because the allowlist this feeds emits ip/32 rules and a v6 entry there is
    // malformed nft that fails the whole apply. Hence each step advances on "no IPv4" rather than "nothing
    // at all": testing the combined result would let a v6-only reply suppress the later sources, discard
    // the v6 anyway, and return empty with no warning.
    std::vector<std::string> v4, v6;
    getOSDefaultDnsServers_Resolvectl(vpnInterfaceName, v4, v6);
    if (v4.empty() && LinuxUtils::isNetworkManagerActive()) {
        getOSDefaultDnsServers_NMCLI(vpnInterfaceName, v4, v6);
    }
    if (v4.empty()) {
        getOSDefaultDnsServers_ResolvConf(tunnelDnsServers, v4, v6);
    }
    if (v4.empty()) {
        // Two causes, and the message must not claim only the first: the sources can have returned no
        // IPv4 resolver, or resolv.conf's strip can have removed the tunnel's own as the last source.
        qCWarning(LOG_FIREWALL_CONTROLLER) << "No OS default IPv4 DNS to allowlist: resolvectl, nmcli and /etc/resolv.conf returned none, or all were the tunnel's own";
    }
    return toWStringList(v4);
}

}
