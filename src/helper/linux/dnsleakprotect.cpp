#include "dnsleakprotect.h"

#include <algorithm>
#include <arpa/inet.h>
#include <fstream>
#include <netinet/in.h>
#include <sstream>
#include <spdlog/spdlog.h>

#include "../common/validation_posix.h"
#include "nftables/nftablescontroller.h"
#include "types/ipaddress.h"
#include "utils.h"

namespace {

void pushUnique(std::vector<std::string> &v, const std::string &s)
{
    if (std::find(v.begin(), v.end(), s) == v.end()) {
        v.push_back(s);
    }
}

// Strip an optional "%zone-id" suffix, then validate + family-classify via types::IpAddress (the
// project's canonical parser): a valid v4 literal lands in v4, a valid v6 literal in v6, anything
// else is dropped. Filtering here means a malformed token can never reach the nft rule text (where it
// would otherwise abort the transaction).
void addIfIp(std::string tok, std::vector<std::string> &v4, std::vector<std::string> &v6)
{
    const auto pct = tok.find('%');
    if (pct != std::string::npos) {
        tok = tok.substr(0, pct);
    }
    if (tok.empty()) {
        return;
    }
    const types::IpAddress addr(tok);
    if (!addr.isValid()) {
        return;
    }
    // Store the canonical inet_ntop form, not the raw token, so removeAllowed's exact-string compare
    // matches allowedDnsServers (also canonical via IpAddress::toString); an IPv6 server printed
    // non-canonically (uppercase / different zero-compression) would otherwise slip the comparison and
    // land its own tunnel resolver in the drop list.
    pushUnique(addr.isV6() ? v6 : v4, addr.toString());
}

// Tokenize a whitespace-separated address list and classify each token (addIfIp). Shared by the
// resolvectl / nmcli / resolv.conf parsers, which all reduce to "addresses after a colon/keyword".
void addTokens(const std::string &list, std::vector<std::string> &v4, std::vector<std::string> &v6)
{
    std::istringstream ts(list);
    std::string tok;
    while (ts >> tok) {
        addIfIp(tok, v4, v6);
    }
}

// Parse `resolvectl dns` output. Each line is "Link N (ifc): addr addr ...". Skip the line whose
// header (before the first colon) names the tunnel interface so we never blacklist our own VPN DNS;
// everything after the first colon is the space-separated address list.
void parseResolvectl(const std::string &out, const std::string &vpnIface,
                     std::vector<std::string> &v4, std::vector<std::string> &v6)
{
    std::istringstream s(out);
    std::string line;
    while (std::getline(s, line)) {
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        const std::string head = line.substr(0, colon);
        if (!vpnIface.empty() && head.find("(" + vpnIface + ")") != std::string::npos) {
            continue;
        }
        addTokens(line.substr(colon + 1), v4, v6);
    }
}

// Parse `nmcli dev show` output: rows "IP4.DNS[n]: <addr>" / "IP6.DNS[n]: <addr>". Classification is
// by actual family (addIfIp), so a value lands in the right list regardless of the row label. The
// output is grouped per device by a leading "GENERAL.DEVICE: <name>" row; track the current device
// and skip the tunnel's own block so we never blacklist our own VPN DNS — the same tunnel-link
// exemption parseResolvectl applies via its header filter.
void parseNmcli(const std::string &out, const std::string &vpnIface,
                std::vector<std::string> &v4, std::vector<std::string> &v6)
{
    std::istringstream s(out);
    std::string line;
    std::string curDev;
    while (std::getline(s, line)) {
        if (line.rfind("GENERAL.DEVICE", 0) == 0) {
            const auto colon = line.find(':');
            if (colon != std::string::npos) {
                std::istringstream ds(line.substr(colon + 1));
                curDev.clear();
                ds >> curDev;
            }
            continue;
        }
        if (line.find("IP4.DNS") == std::string::npos && line.find("IP6.DNS") == std::string::npos) {
            continue;
        }
        if (!vpnIface.empty() && curDev == vpnIface) {
            continue;
        }
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        addTokens(line.substr(colon + 1), v4, v6);
    }
}

// Last-resort fallback: parse /etc/resolv.conf "nameserver <addr>" lines. Used only when neither
// resolvectl nor nmcli reported any resolver (a host with a static resolv.conf and no
// systemd-resolved / NetworkManager), so the OS-default resolver is still captured rather than left
// reachable outside the tunnel. The tunnel's own resolver (which the DNS manager may have written
// here) is stripped afterward by removeAllowed, and loopback is skipped at rule-emit time.
void parseResolvConf(std::vector<std::string> &v4, std::vector<std::string> &v6)
{
    std::ifstream f("/etc/resolv.conf");
    std::string line;
    while (std::getline(f, line)) {
        std::istringstream ls(line);
        std::string key;
        if (!(ls >> key) || key != "nameserver") {
            continue;
        }
        std::string rest;
        std::getline(ls, rest);
        addTokens(rest, v4, v6);
    }
}

// Collect every IPv4 default-route gateway from /proc/net/route, skipping routes on the tunnel
// interface. Many routers (and multi-WAN hosts with more than one default gateway) act as DNS
// forwarders on the gateway address without advertising it, so each must be blocked, not just one.
void addDefaultGatewaysV4(const std::string &vpnIface, std::vector<std::string> &v4)
{
    std::ifstream f("/proc/net/route");
    std::string line;
    std::getline(f, line); // header row
    while (std::getline(f, line)) {
        std::istringstream ls(line);
        std::string iface, dest, gateway;
        if (!(ls >> iface >> dest >> gateway)) {
            continue;
        }
        if (dest != "00000000") {
            continue; // only default routes (destination 0.0.0.0)
        }
        if (!vpnIface.empty() && iface == vpnIface) {
            continue;
        }
        // Gateway is little-endian hex; on this LE platform the parsed word is already the in_addr.
        in_addr addr;
        try {
            addr.s_addr = static_cast<in_addr_t>(std::stoul(gateway, nullptr, 16));
        } catch (...) {
            continue;
        }
        if (addr.s_addr == 0) {
            continue;
        }
        char buf[INET_ADDRSTRLEN] = {};
        if (inet_ntop(AF_INET, &addr, buf, sizeof(buf)) != nullptr) {
            pushUnique(v4, buf);
        }
    }
}

void removeAllowed(std::vector<std::string> &servers, const std::vector<std::string> &allowed)
{
    servers.erase(std::remove_if(servers.begin(), servers.end(),
                                 [&](const std::string &ip) {
                                     return std::find(allowed.begin(), allowed.end(), ip) != allowed.end();
                                 }),
                  servers.end());
}

} // namespace

namespace DnsLeakProtect {

bool enable(const std::string &vpnInterfaceName, const std::vector<std::string> &allowedDnsServers,
            const std::string &defaultGatewayV4)
{
    std::string iface = vpnInterfaceName;
    if (!iface.empty() && !Validation::isValidInterfaceName(iface)) {
        spdlog::warn("DnsLeakProtect: invalid tunnel interface name \"{}\"; ignoring it", iface);
        iface.clear();
    }

    std::vector<std::string> v4, v6;

    // Snapshot the OS-default resolvers. Prefer resolvectl; fall back to nmcli when resolvectl is
    // unavailable OR left EITHER family empty.
    //
    // The fallback is deliberately per-family (v4 empty OR v6 empty), not all-or-nothing. resolvectl
    // only reports what systemd-resolved was fed; NetworkManager and systemd-resolved are independent
    // sources. A dual-stack host can have its v4 DNS in resolved (so resolvectl shows v4) while its v6
    // resolver — e.g. one learned from RA RDNSS — is known only to NetworkManager and never pushed to
    // resolved. With an all-or-nothing fallback (v4 && v6 empty) the non-empty v4 result suppresses the
    // nmcli query, that v6 resolver is never snapshotted, and v6 DNS to it leaks outside the tunnel —
    // exactly the leak this chain exists to stop. Querying nmcli whenever a family is missing closes
    // that gap; parseNmcli's addIfIp dedups (pushUnique) so re-confirming an already-populated family is
    // harmless, and the extra subprocess on a genuinely v4-only-DNS host is a cheap price for not
    // silently dropping v6 leak protection. Do NOT "optimize" this back to v4 && v6.
    auto snapshotNmcli = [&]() {
        std::string nm;
        Utils::executeCommand("nmcli", {"dev", "show"}, &nm);
        parseNmcli(nm, iface, v4, v6);
    };

    std::string out;
    if (Utils::executeCommand("resolvectl", {"dns"}, &out) != 0) {
        snapshotNmcli();
    } else {
        parseResolvectl(out, iface, v4, v6);
        if (v4.empty() || v6.empty()) {
            snapshotNmcli();
        }
    }

    // Neither resolvectl nor nmcli yielded a resolver (e.g. a static /etc/resolv.conf with no
    // systemd-resolved / NetworkManager). Fall back to resolv.conf so an OS-default resolver still
    // lands in the drop list instead of leaking outside the tunnel.
    if (v4.empty() && v6.empty()) {
        parseResolvConf(v4, v6);
    }

    // Capture whether resolver discovery turned up anything BEFORE the default gateways are added below:
    // the gateways populate v4 and would otherwise mask a genuine "no resolvers discovered" condition.
    const bool noResolversFound = v4.empty() && v6.empty();

    // Block the physical default gateways too — many home routers act as DNS forwarders on the gateway
    // address without advertising it. The caller supplies the pre-VPN gateway, and we also enumerate
    // every IPv4 default-route nexthop still present (a multi-WAN host can have more than one), skipping
    // the tunnel's own default route. No v6 equivalent: the v6 default nexthop is the router's
    // link-local, virtually never a DNS server, and real v6 forwarders show up via RA RDNSS above.
    if (types::IpAddress(defaultGatewayV4).isValid()) {
        pushUnique(v4, defaultGatewayV4);
    }
    addDefaultGatewaysV4(iface, v4);

    // Never blacklist the tunnel's own DNS servers. Canonicalize the caller's list HERE (the single
    // place every caller's allowed set funnels through) so the exact-string compare in removeAllowed
    // can't miss: the snapshot is stored canonical (addIfIp -> IpAddress::toString), so the allowed
    // entries must be canonical too, else a non-canonical token (uppercase / different IPv6
    // zero-compression) would slip past removeAllowed and land the tunnel's own resolver in the drop
    // list. Invalid tokens are dropped rather than rejecting the whole command.
    std::vector<std::string> allowed;
    for (const auto &ip : allowedDnsServers) {
        const types::IpAddress parsed(ip);
        if (parsed.isValid()) {
            allowed.push_back(parsed.toString());
        } else {
            spdlog::warn("DnsLeakProtect: dropping invalid allowed DNS server \"{}\"", ip);
        }
    }
    removeAllowed(v4, allowed);
    removeAllowed(v6, allowed);

    // Surface a failed resolver discovery rather than letting it pass silently. The usual cause is none
    // of resolvectl, nmcli, or /etc/resolv.conf being available (or returning) any resolvers; only the
    // physical default gateway(s), if any, end up blocked.
    if (noResolversFound) {
        spdlog::warn("DnsLeakProtect: no OS-default resolvers found (resolvectl/nmcli/resolv.conf "
                     "returned none); only the physical default gateway(s) will be blocked");
    }


    // Create the dnsleaks base chain if absent (ahead of the firewall, accept policy so it only ever
    // drops leaking DNS), then flush its rules (see nft::ensureChain).
    std::string buf = nft::addTable();
    buf += nft::ensureChain("dnsleaks", "type filter hook output priority -20; policy accept;");

    // Accept loopback (local resolver). Intentionally NO interface-based tunnel exemption — and do not
    // re-add one. The old shell design emitted `-o <vpn_iface> -j RETURN`, which was a bug: under a
    // full-tunnel route the physical link's OS resolvers are reachable THROUGH the tunnel, so an
    // egress-interface RETURN let queries to them leave via the VPN exit — exactly the leak this chain
    // exists to stop. The correct and only mechanism is destination-based: strip the tunnel's own
    // resolvers up front via allowedDnsServers (done above), then drop every remaining OS resolver
    // regardless of egress interface, which forces all DNS onto the VPN resolver. The allowedDnsServers
    // strip is therefore load-bearing by design; both it and the snapshot use IpAddress::toString() so
    // the exact-match in removeAllowed() can never miss the tunnel resolver.
    buf += nft::rule("dnsleaks", "oifname \"lo\" accept");

    auto emitDrops = [&buf](const std::string &fam, const std::vector<std::string> &ips) {
        for (const auto &ip : ips) {
            // Skip the loopback/unspecified stub resolver (the combined predicate is family-safe:
            // a v4 list never holds ::1/:: and a v6 list never holds 127./0.0.0.0).
            if (ip.rfind("127.", 0) == 0 || ip == "0.0.0.0" || ip == "::1" || ip == "::") {
                continue;
            }
            buf += nft::rule("dnsleaks", fam + " daddr " + ip + " meta l4proto { tcp, udp } th dport 53 drop");
        }
    };
    emitDrops("ip", v4);
    emitDrops("ip6", v6);

    return NftablesController::instance().run(buf);
}

bool disable()
{
    return NftablesController::instance().run(nft::addTable() + nft::deleteChain("dnsleaks"));
}

} // namespace DnsLeakProtect
