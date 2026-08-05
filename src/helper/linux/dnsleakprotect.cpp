#include "dnsleakprotect.h"

#include <arpa/inet.h>
#include <fstream>
#include <netinet/in.h>
#include <sstream>
#include <spdlog/spdlog.h>

#include "../common/validation_posix.h"
#include "nftables/nftablescontroller.h"
#include "types/ipaddress.h"
#include "utils.h"
#include "utils/dns_utils/dnsleakparse.h"

namespace {

// A rejected token means a resolver silently missed the drop list, so it must be visible. Plain spdlog
// is correct here; the client half of DnsLeakParse installs its own sink.
void warnToken(const std::string &tok)
{
    spdlog::warn("DnsLeakProtect: skipping unusable DNS server token \"{}\"", tok);
}

// Collect every IPv4 default-route gateway from /proc/net/route, skipping routes on the tunnel
// interface. Many routers (and multi-WAN hosts with more than one default gateway) act as DNS
// forwarders on the gateway address without advertising it, so each must be blocked, not just one.
void addDefaultGatewaysV4(const std::string &vpnIface, std::vector<std::string> &v4,
                          std::vector<std::string> &v6)
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
            // v6 is passed only because addAddress classifies by family; AF_INET output cannot land there.
            DnsLeakParse::addAddress(buf, v4, v6, warnToken);
        }
    }
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
    // that gap; parseNmcli's addAddress dedups, so re-confirming an already-populated family is
    // harmless, and the extra subprocess on a genuinely v4-only-DNS host is a cheap price for not
    // silently dropping v6 leak protection. Do NOT "optimize" this back to v4 && v6.
    auto snapshotNmcli = [&]() {
        // nmcli reaches NetworkManager over D-Bus. With NM not running the call yields nothing and can
        // stall on activation — and from root it would D-Bus-activate a NetworkManager the user
        // deliberately disabled. Every other nmcli call in this process is gated the same way.
        if (!Utils::isNetworkManagerActive()) {
            return;
        }
        std::string nm;
        // Discard a failed run rather than parsing what it wrote: a timeout kill leaves a PREFIX (nmcli
        // emits a block per device, stdio flushes in blocks), which would lose the tail devices' resolvers
        // AND, being non-empty, decline the resolv.conf fallback and suppress the no-resolvers warning.
        const int rc = Utils::executeCommand("timeout", {"2", "nmcli", "dev", "show"}, &nm, false);
        if (rc != 0) {
            spdlog::warn("DnsLeakProtect: nmcli dev show failed (exit {}); ignoring its output", rc);
            return;
        }
        DnsLeakParse::parseNmcli(nm, iface, v4, v6, warnToken);
    };

    // `timeout` because enable() runs on the helper's single io_context thread, so a wedged resolved or
    // D-Bus would stop EVERY privileged command, including firewall-off and disable(). Exit 124 then
    // classifies as failure below and falls through to the next source.
    //
    // appendFromStdErr=false because it defaults to true, and executeCommand requires marker-based
    // parsing for merged output — which parseResolvectl does not do. A diagnostic between an exempt
    // tunnel header and its continuation would clear the exemption and snapshot the tunnel's resolvers.
    std::string out;
    const int resolvectlRc = Utils::executeCommand("timeout", {"2", "resolvectl", "dns"}, &out, false);
    // Discard only when the text was cut mid-line: 124 is timeout's own kill, and >=128 is a signalled
    // child — executeCommand waits on the enclosing shell, so a signalled resolvectl arrives as 128+sig
    // rather than as the -1 that only a signalled shell produces. Any other non-zero is resolvectl's own:
    // status_all prints every link it read and returns the first error, so that output is complete for
    // the links it covers and is worth parsing rather than discarding wholesale.
    if (resolvectlRc == 124 || resolvectlRc < 0 || resolvectlRc >= 128) {
        spdlog::warn("DnsLeakProtect: resolvectl dns was killed (exit {}); ignoring its output", resolvectlRc);
        snapshotNmcli();
    } else {
        if (resolvectlRc != 0) {
            spdlog::warn("DnsLeakProtect: resolvectl dns exited {}; using the links it did report", resolvectlRc);
        }
        DnsLeakParse::parseResolvectl(out, iface, v4, v6, {}, warnToken);
        if (v4.empty() || v6.empty()) {
            snapshotNmcli();
        }
    }

    // Per-family for the same reason the nmcli gate above is, and the client's chain already is: an
    // address family missing from the sources above can still be in resolv.conf, and an all-or-nothing
    // gate would skip it and leave that resolver unblocked while the client allowlists it. Being flat,
    // its entries are stripped by value below rather than by interface.
    if (v4.empty() || v6.empty()) {
        if (!DnsLeakParse::parseResolvConfFile("/etc/resolv.conf", v4, v6, warnToken)) {
            spdlog::warn("DnsLeakProtect: /etc/resolv.conf could not be read; no resolvers from it");
        }
    }

    // Strip our own resolvers BEFORE counting. The flag means "no OS-default resolver we would block", and
    // the tunnel's resolver can be in the snapshot legitimately — the DNS manager writes it into the flat
    // resolv.conf we may just have read — so counting it makes an inert chain look like a populated one.
    DnsLeakParse::removeMatching(v4, allowedDnsServers, warnToken);
    DnsLeakParse::removeMatching(v6, allowedDnsServers, warnToken);

    // Captured BEFORE the gateways are added below, so they cannot mask a snapshot that held nothing
    // blockable. It means "nothing left to block", not "nothing discovered" — the strip above may have
    // removed everything the sources found.
    const bool noResolversToBlock = v4.empty() && v6.empty();

    // Block the physical default gateways too — many home routers act as DNS forwarders on the gateway
    // address without advertising it. The caller supplies the pre-VPN gateway, and we also enumerate
    // every IPv4 default-route nexthop still present (a multi-WAN host can have more than one), skipping
    // the tunnel's own default route. No v6 equivalent: the v6 default nexthop is the router's
    // link-local, virtually never a DNS server, and real v6 forwarders show up via RA RDNSS above.
    //
    // Routed through addAddress rather than isValid() plus a raw append: isValid() only means "parses as
    // SOME family", so a v6 value would have landed in the v4 list and rendered `ip daddr <v6>` —
    // malformed nft that fails the transaction and leaves NO drop rules at all.
    DnsLeakParse::addAddress(defaultGatewayV4, v4, v6, warnToken);
    addDefaultGatewaysV4(iface, v4, v6);

    // Again after the gateways, and not merely for symmetry: allowedDnsServers carries dnsWhitelistIps,
    // ctrld's legacy RFC1918 upstream, which can BE the LAN router that addDefaultGatewaysV4 just pushed.
    // Stripping only before the gateway add would leave that address in the drop list. Passing no sink
    // here so an invalid allowed entry is reported once, by the first pass.
    DnsLeakParse::removeMatching(v4, allowedDnsServers);
    DnsLeakParse::removeMatching(v6, allowedDnsServers);

    // Surface an empty resolver snapshot rather than letting it pass silently. Both causes matter and the
    // message must not claim only the first: the sources can genuinely have returned nothing, or they can
    // have returned resolvers that are all on the allow list (ctrld's plain-DNS upstream is a common one).
    if (noResolversToBlock) {
        spdlog::warn("DnsLeakProtect: no OS-default resolver left to block (sources returned none, or all "
                     "were on the allow list); only default gateway(s) not themselves allowed get blocked");
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
    // strip is therefore load-bearing by design; both sides canonicalize via IpAddress::toString(), and
    // removeMatching compares only the address half of a packed entry, so the exemption covers every port.
    buf += nft::rule("dnsleaks", "oifname \"lo\" accept");

    auto emitDrops = [&buf](const std::string &fam, const std::vector<std::string> &entries) {
        for (const auto &entry : entries) {
            std::string ip;
            uint16_t port = 0;
            if (!types::splitEndpoint(entry, ip, port)) {
                spdlog::warn("DnsLeakProtect: skipping unparseable snapshot entry \"{}\"", entry);
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
