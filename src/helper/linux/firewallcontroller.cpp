#include "firewallcontroller.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sstream>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "../common/helper_commands.h"
#include "../common/io_posix.h"
#include "../common/validation_posix.h"
#include "split_tunneling/cgroups.h"
#include "utils.h"

FirewallController::FirewallController() : connected_(false), splitTunnelEnabled_(false), splitTunnelExclude_(true)
{
}

FirewallController::~FirewallController()
{
}

bool FirewallController::enable(const FirewallConfig &config)
{
    // Probe each family independently for our INPUT jump. The engine installs v4 and v6 together,
    // but if one family's jump was torn down out-of-band (e.g. an external `ip6tables -F INPUT`) a
    // shared either-family probe would skip re-inserting the missing jump and leave that family's
    // chains unreferenced — fail-open for that family. Probing per family re-inserts only what's
    // missing.
    const bool v4Exists = inputJumpExists(false);

    // Skip IPv6 entirely when the kernel has no IPv6 stack (ipv6.disable=1 or the module isn't
    // loaded): ip6tables-restore can't initialize its filter table there, so a v6 attempt would fail
    // every time and — now that the engine acts on enable()'s result — doom the whole kill switch
    // into a retry loop even though v4 applied cleanly. With no v6 stack there is nothing to leak, so
    // dropping the v6 ruleset is safe. When v6 IS available its rules are mandatory and a v6 failure
    // is a real, retryable error.
    //
    // This is evaluated only when enable() runs. If the v6 stack is brought up after the firewall is
    // already enabled (module loaded later, ipv6.disable toggled), the v6 family stays unfiltered
    // until the next firewall apply (reconnect / interface refresh / setting change) re-runs enable().
    const bool v6 = ipv6Available();
    const bool v6Exists = v6 && inputJumpExists(true);

    // Probe the VPN adapter once (single getifaddrs walk) and feed both builders: the v4 builder
    // needs its local v4 addresses, the v6 builder needs only whether a non-link-local v6 address
    // is present.
    std::vector<std::string> localV4Addrs;
    bool hasV6Addr = false;
    if (!config.vpnInterfaceName.empty()) {
        probeInterfaceAddresses(config.vpnInterfaceName, localV4Addrs, hasV6Addr);
    }

    // Build the families from the validated intent. Rule text never crosses the IPC boundary; the
    // helper is the sole author.
    if (!writeRulesFile(false, buildRulesV4(config, v4Exists, localV4Addrs))) {
        return false;
    }
    if (v6 && !writeRulesFile(true, buildRulesV6(config, v6Exists, hasV6Addr))) {
        return false;
    }

    // Validate BOTH families before committing EITHER. iptables-restore is atomic per call, but the
    // two families are separate calls: applying v4 and only then discovering v6 is malformed would
    // leave an inconsistent kill switch (v4 enforced while v6 leaks). The --test pass parses and
    // constructs each ruleset without committing it, so a rejected ruleset (version mismatch, a token
    // that slipped validation) is caught before anything is applied, leaving existing rules intact.
    // Mirrors the macOS pfctl -n pre-parse.
    if (!restoreRules(false, /*testOnly=*/true) || (v6 && !restoreRules(true, /*testOnly=*/true))) {
        spdlog::error("Generated firewall ruleset failed validation; leaving existing rules intact");
        return false;
    }

    // Both validated; commit them. iptables has no cross-family transaction, so a transient runtime
    // failure (e.g. an xtables lock acquired between the test and the commit) can apply one family
    // and not the other. Return false so the engine retries the full apply, and leave the family that
    // did apply in place: keeping more of the kill switch enforced is the safe direction, and the
    // retry converges both families.
    const bool v4Ok = restoreRules(false, /*testOnly=*/false);
    const bool v6Ok = v6 ? restoreRules(true, /*testOnly=*/false) : true;

    // The restore blobs flushed our chains, wiping any per-chain rules the split-tunnel pipeline
    // previously appended (cgroup ACCEPTs, IP exceptions, ingress connmark). Re-emit them only once
    // both families' base rulesets are back in place, since these helpers append to both v4 and v6
    // chains and appending to a family a failed restore didn't (re)install would just error; the
    // retry restores them. Skip the re-emit entirely unless split tunneling is active: the flush
    // already wiped the chain rules and there is nothing to restore, so the three calls would only
    // spawn a handful of no-op iptables/ip6tables -D deletes on every firewall apply (the common
    // no-split-tunnel path).
    if (v4Ok && v6Ok && connected_ && splitTunnelEnabled_) {
        setSplitTunnelIpExceptions(splitTunnelIps_);
        setSplitTunnelAppExceptions();
        setSplitTunnelIngressRules(defaultAdapterIp_, defaultAdapterIpV6_);
    }

    return v4Ok && v6Ok;
}

bool FirewallController::ipv6Available()
{
    // /proc/net/if_inet6 is present whenever the kernel has an active IPv6 stack (it lists the v6
    // interface addresses, always including loopback ::1). It is absent when IPv6 is disabled via
    // ipv6.disable=1 or the ipv6 module isn't loaded, which is exactly when ip6tables can't operate.
    std::error_code ec;
    return std::filesystem::exists("/proc/net/if_inet6", ec);
}

bool FirewallController::writeRulesFile(bool ipv6, const std::string &rules)
{
    const char *path = ipv6 ? WS_LINUX_RUN_DIR "/rules.v6" : WS_LINUX_RUN_DIR "/rules.v4";
    if (!IO::writeFile(path, rules, S_IRUSR | S_IWUSR)) {
        spdlog::error("Could not write firewall rules: {}", IO::strerror(errno));
        return false;
    }
    return true;
}

bool FirewallController::restoreRules(bool ipv6, bool testOnly)
{
    const char *path = ipv6 ? WS_LINUX_RUN_DIR "/rules.v6" : WS_LINUX_RUN_DIR "/rules.v4";
    const char *tool = ipv6 ? "ip6tables-restore" : "iptables-restore";

    // -n (noflush): our blob is a partial ruleset that must preserve the built-in INPUT/OUTPUT and
    // any third-party rules; it explicitly flushes only our own chains. --test additionally parses
    // and constructs the ruleset without committing it.
    std::vector<std::string> args = {"-n"};
    if (testOnly) {
        args.push_back("--test");
    }
    args.push_back(path);

    const int rc = Utils::executeCommand(tool, args);
    if (rc != 0) {
        // A rejected restore (version incompatibility, a token that slipped validation, a transient
        // lock failure) leaves our chains in their prior state. Report it so enable() returns false
        // and the engine does not mark the kill switch active while traffic flows unfiltered.
        spdlog::error("{} {} firewall rules (exit {})", tool, testOnly ? "failed to validate" : "failed to load", rc);
        return false;
    }
    return true;
}

bool FirewallController::hasBlockJump(const char *chain)
{
    return Utils::executeCommand("iptables", {"--check", chain, "-j", WS_PRODUCT_NAME_LOWER "_block", "-m", "comment", "--comment", kTag.c_str()}) == 0;
}

int FirewallController::dnsLeaksJumpPos(bool isIPv6, bool tunnelAdapterSet)
{
    // 1-based position of the dns-leak-protect OUTPUT jump (`-j windscribe_dnsleaks`, installed by
    // the dns-leak-protect script at connect time) within the OUTPUT chain, or 0 if absent. The
    // caller inserts windscribe_output directly *below* this position so the dnsleaks DROP rules
    // keep precedence over windscribe_output's tunnel/LAN ACCEPTs. Computing the real index (rather
    // than assuming position 1) keeps the ordering correct even when third-party rules sit above it.
    //
    // KNOWN RESIDUAL TOCTOU WINDOW: this read and the subsequent restore that applies the
    // `-I OUTPUT P+1` jump are separate operations, while dns-leak-protect runs out-of-process and
    // is driven asynchronously by OpenVPN/DNS-manager up/down events. A DNS refresh can fire its
    // down->up cycle between our read and our apply. This is mostly self-correcting because
    // dns-leak-protect ALWAYS top-inserts the dnsleaks jump at position 1: any windscribe_output we
    // insert at P+1 (>= 2) still lands below a freshly re-added jump, so the DROPs keep precedence.
    // The one un-covered case: the read observes the jump transiently absent (mid down-phase) -> we
    // return 0 -> top insert -> the up re-adds dnsleaks above windscribe_output -> brief leak. The
    // exposure is narrow (only on firewall enable coinciding with a DNS refresh on an
    // already-connected tunnel), so this is left documented rather than synchronized.
    const std::string saveCmd = isIPv6 ? "ip6tables-save" : "iptables-save";
    std::string rules;
    bool readOk = false;
    for (int attempt = 0; attempt < 3; ++attempt) {
        rules.clear();
        // A genuinely successful save always emits a "*filter" section; use its presence (and a
        // zero exit) to detect a failed/empty read and retry. This matters because the
        // leak-producing choice (top-inserting windscribe_output above the dnsleaks jump) is exactly
        // what a misdetected "no dnsleaks" read would cause.
        const int rc = Utils::executeCommand(saveCmd, {}, &rules, /*appendFromStdErr=*/false);
        if (rc == 0 && rules.find("*filter") != std::string::npos) {
            readOk = true;
            break;
        }
        usleep(50 * 1000);
    }

    if (!readOk) {
        // Couldn't read the table after retries — keep this distinct from a clean "no dnsleaks jump"
        // result (return 0 -> top insert). Top-inserting here would shadow the dnsleaks DROPs and
        // leak OS-default DNS whenever the jump is in fact present. dns-leak-protect always
        // top-inserts its OUTPUT jump (position 1) on VPN up, so when a tunnel adapter is set
        // (VPN connected -> jump almost certainly present) report position 1; the caller then
        // inserts windscribe_output directly below it. Disconnected (no tunnel adapter) the jump is
        // absent, so report 0 and let the caller do a valid top insert.
        spdlog::warn("dnsLeaksJumpPos: failed to read {} rules after retries; using conservative fallback",
                     isIPv6 ? "IPv6" : "IPv4");
        return tunnelAdapterSet ? 1 : 0;
    }

    // iptables-save prints each chain's rules in kernel order, so counting the -A OUTPUT lines up to
    // and including the dnsleaks jump yields its 1-based position. Matching on the chain name (not
    // the comment) keeps this robust to iptables-nft comment requoting, the same reason the
    // dns-leak-protect script matches loosely.
    int pos = 0;
    std::istringstream stream(rules);
    std::string line;
    while (std::getline(stream, line)) {
        // Trailing space anchors on the OUTPUT chain exactly, so a custom chain named e.g.
        // "OUTPUT_custom" doesn't get counted into OUTPUT's position.
        if (line.rfind("-A OUTPUT ", 0) == 0) {
            ++pos;
            if (line.find("-j " WS_PRODUCT_NAME_LOWER "_dnsleaks") != std::string::npos) {
                return pos;
            }
        }
    }
    return 0;
}

std::string FirewallController::outputJumpRule(bool isIPv6, bool tunnelAdapterSet, const std::string &commentSuffix)
{
    // Insert windscribe_output directly below the dns-leak-protect jump so the dnsleaks DROP rules
    // keep precedence over our tunnel/LAN ACCEPTs; a plain top insert would shadow them and leak
    // OS-default DNS. When the dnsleaks jump is absent (firewall enabled while disconnected) fall
    // back to a top insert. Single owner of the OUTPUT-jump ordering for both address families so a
    // future fix can never be applied to only one family and silently re-open the leak.
    const int dnsLeaksPos = dnsLeaksJumpPos(isIPv6, tunnelAdapterSet);
    const std::string pos = dnsLeaksPos > 0 ? std::to_string(dnsLeaksPos + 1) + " " : std::string();
    return "-I OUTPUT " + pos + "-j " WS_PRODUCT_NAME_LOWER "_output" + commentSuffix;
}

std::string FirewallController::buildRulesV4(const FirewallConfig &config, bool bExists, const std::vector<std::string> &localV4Addrs)
{
    const std::string &iface = config.vpnInterfaceName;
    const std::string hotspotAdapter = getHotspotAdapter();
    const std::string c = " -m comment --comment \"" + kTag + "\"\n";

    std::string r;
    r += "*filter\n";
    r += ":" WS_PRODUCT_NAME_LOWER "_input - [0:0]\n";
    r += ":" WS_PRODUCT_NAME_LOWER "_output - [0:0]\n";
    r += ":" WS_PRODUCT_NAME_LOWER "_block - [0:0]\n";

    // `iptables-restore -n` does not flush chains declared via `:CHAIN` — it only creates them if
    // absent and zeros counters; existing rules stay. Flush explicitly so the blob is authoritative
    // for its own chains without disturbing INPUT/OUTPUT jumps or third-party rules.
    r += "-F " WS_PRODUCT_NAME_LOWER "_input\n";
    r += "-F " WS_PRODUCT_NAME_LOWER "_output\n";
    r += "-F " WS_PRODUCT_NAME_LOWER "_block\n";

    if (!bExists) {
        r += "-I INPUT -j " WS_PRODUCT_NAME_LOWER "_input" + c;
        // On a late firewall enable while the VPN is already connected the dns-leak-protect OUTPUT
        // jump already exists. Insert our OUTPUT jump directly *below* it so the dnsleaks DROP rules
        // keep precedence over windscribe_output's tunnel/LAN ACCEPTs; a plain top insert would
        // shadow them and leak OS-default DNS. When the dnsleaks jump is absent (firewall enabled
        // while disconnected) outputJumpRule falls back to a top insert.
        r += outputJumpRule(false, !iface.empty(), c);
    }

    // Probe each chain's block jump independently and re-add only the missing one. A combined
    // INPUT-only probe would skip re-adding an OUTPUT jump that was torn down out-of-band, leaving
    // windscribe_output with no terminal DROP (egress falls through to the chain's ACCEPT policy and
    // leaks); probing per chain also avoids appending a duplicate jump to a chain that still has one,
    // since iptables-restore -n does not dedupe -A on the built-in INPUT/OUTPUT chains.
    if (!hasBlockJump("INPUT")) {
        r += "-A INPUT -j " WS_PRODUCT_NAME_LOWER "_block" + c;
    }
    if (!hasBlockJump("OUTPUT")) {
        r += "-A OUTPUT -j " WS_PRODUCT_NAME_LOWER "_block" + c;
    }

    r += "-A " WS_PRODUCT_NAME_LOWER "_input -i lo -j ACCEPT" + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_output -o lo -j ACCEPT" + c;

    r += "-A " WS_PRODUCT_NAME_LOWER "_input -p udp --sport 67:68 --dport 67:68 -j ACCEPT" + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_output -p udp --sport 67:68 --dport 67:68 -j ACCEPT" + c;

    if (!iface.empty()) {
        if (!config.isCustomConfig) {
            // Allow local addresses
            for (const auto &addr : localV4Addrs) {
                r += "-A " WS_PRODUCT_NAME_LOWER "_input -i " + iface + " -s " + addr + "/32 -j ACCEPT" + c;
                r += "-A " WS_PRODUCT_NAME_LOWER "_output -o " + iface + " -d " + addr + "/32 -j ACCEPT" + c;
            }

            // Disallow LAN addresses (except 10.255.255.0/24), link-local addresses, loopback,
            // and local multicast addresses from going into the tunnel
            r += "-A " WS_PRODUCT_NAME_LOWER "_input -i " + iface + " -s 192.168.0.0/16 -j DROP" + c;
            r += "-A " WS_PRODUCT_NAME_LOWER "_output -o " + iface + " -d 192.168.0.0/16 -j DROP" + c;
            r += "-A " WS_PRODUCT_NAME_LOWER "_input -i " + iface + " -s 172.16.0.0/12 -j DROP" + c;
            r += "-A " WS_PRODUCT_NAME_LOWER "_output -o " + iface + " -d 172.16.0.0/12 -j DROP" + c;
            r += "-A " WS_PRODUCT_NAME_LOWER "_input -i " + iface + " -s 169.254.0.0/16 -j DROP" + c;
            r += "-A " WS_PRODUCT_NAME_LOWER "_output -o " + iface + " -d 169.254.0.0/16 -j DROP" + c;
            r += "-A " WS_PRODUCT_NAME_LOWER "_input -i " + iface + " -s 10.255.255.0/24 -j ACCEPT" + c;
            r += "-A " WS_PRODUCT_NAME_LOWER "_output -o " + iface + " -d 10.255.255.0/24 -j ACCEPT" + c;
            r += "-A " WS_PRODUCT_NAME_LOWER "_input -i " + iface + " -s 10.0.0.0/8 -j DROP" + c;
            r += "-A " WS_PRODUCT_NAME_LOWER "_output -o " + iface + " -d 10.0.0.0/8 -j DROP" + c;
            r += "-A " WS_PRODUCT_NAME_LOWER "_input -i " + iface + " -s 224.0.0.0/4 -j DROP" + c;
            r += "-A " WS_PRODUCT_NAME_LOWER "_output -o " + iface + " -d 224.0.0.0/4 -j DROP" + c;
        }

        r += "-A " WS_PRODUCT_NAME_LOWER "_input -i " + iface + " -j ACCEPT" + c;
        r += "-A " WS_PRODUCT_NAME_LOWER "_output -o " + iface + " -j ACCEPT" + c;

        // accept filter for the hotspot adapter in the connected state
        if (!hotspotAdapter.empty()) {
            r += "-A " WS_PRODUCT_NAME_LOWER "_input -i " + hotspotAdapter + " -j ACCEPT" + c;
            r += "-A " WS_PRODUCT_NAME_LOWER "_output -o " + hotspotAdapter + " -j ACCEPT" + c;
        }
    }

    if (!config.connectingIp.empty()) {
        const std::string &ip = config.connectingIp;
        r += "-A " WS_PRODUCT_NAME_LOWER "_input -s " + ip + "/32 -j ACCEPT" + c;
        // Allow packets from gid 0
        r += "-A " WS_PRODUCT_NAME_LOWER "_output -d " + ip + "/32 -j ACCEPT -m owner --gid-owner 0" + c;
        // Allow packets from app group
        r += "-A " WS_PRODUCT_NAME_LOWER "_output -d " + ip + "/32 -j ACCEPT -m owner --gid-owner " WS_PRODUCT_NAME_LOWER + c;
        // Allow packets from kernel (no uid), for wireguard control traffic
        r += "-A " WS_PRODUCT_NAME_LOWER "_output -d " + ip + "/32 -j ACCEPT -m owner ! --uid-owner 0-4294967294" + c;
        // Allow marked packets.  These packets are marked by the wireguard adapter code.
        // This is necessary because wg packets have uid/gid of the app that created the packet, not wg itself.
        r += "-A " WS_PRODUCT_NAME_LOWER "_output -d " + ip + "/32 -j ACCEPT -m mark --mark 51820" + c;
        r += "-A " WS_PRODUCT_NAME_LOWER "_output -d " + ip + "/32 -j ACCEPT -m mark --mark 20310" + c;
    }

    for (const auto &i : config.allowedIps) {
        r += "-A " WS_PRODUCT_NAME_LOWER "_input -s " + i + "/32 -j ACCEPT" + c;
        r += "-A " WS_PRODUCT_NAME_LOWER "_output -d " + i + "/32 -j ACCEPT" + c;
    }

    // drop filter for the hotspot adapter in the disconnected state
    if (!hotspotAdapter.empty()) {
        r += "-A " WS_PRODUCT_NAME_LOWER "_input -i " + hotspotAdapter + " -j DROP" + c;
        r += "-A " WS_PRODUCT_NAME_LOWER "_output -o " + hotspotAdapter + " -j DROP" + c;
    }

    // Loopback addresses to the local host
    r += "-A " WS_PRODUCT_NAME_LOWER "_input -s 127.0.0.0/8 -j ACCEPT" + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_output -d 127.0.0.0/8 -j ACCEPT" + c;

    // Local Network
    // Custom configs need private ranges allowed so third-party VPN DNS/gateway/routes work
    const std::string action = (config.allowLanTraffic || config.isCustomConfig) ? "ACCEPT" : "DROP";
    r += "-A " WS_PRODUCT_NAME_LOWER "_input -s 192.168.0.0/16 -j " + action + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_output -d 192.168.0.0/16 -j " + action + c;

    r += "-A " WS_PRODUCT_NAME_LOWER "_input -s 172.16.0.0/12 -j " + action + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_output -d 172.16.0.0/12 -j " + action + c;

    r += "-A " WS_PRODUCT_NAME_LOWER "_input -s 169.254.0.0/16 -j " + action + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_output -d 169.254.0.0/16 -j " + action + c;

    r += "-A " WS_PRODUCT_NAME_LOWER "_input -s 10.255.255.0/24 -j DROP" + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_output -d 10.255.255.0/24 -j DROP" + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_input -s 10.0.0.0/8 -j " + action + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_output -d 10.0.0.0/8 -j " + action + c;

    // Multicast addresses
    r += "-A " WS_PRODUCT_NAME_LOWER "_input -s 224.0.0.0/4 -j " + action + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_output -d 224.0.0.0/4 -j " + action + c;

    r += "-A " WS_PRODUCT_NAME_LOWER "_block -j DROP" + c;
    r += "COMMIT\n";
    return r;
}

std::string FirewallController::buildRulesV6(const FirewallConfig &config, bool bExists, bool hasV6Addr)
{
    const std::string &iface = config.vpnInterfaceName;
    const std::string c = " -m comment --comment \"" + kTag + "\"\n";

    std::string r;
    r += "*filter\n";
    r += ":" WS_PRODUCT_NAME_LOWER "_input - [0:0]\n";
    r += ":" WS_PRODUCT_NAME_LOWER "_output - [0:0]\n";

    // See buildRulesV4: ip6tables-restore -n does not flush chains declared via `:CHAIN`.
    r += "-F " WS_PRODUCT_NAME_LOWER "_input\n";
    r += "-F " WS_PRODUCT_NAME_LOWER "_output\n";

    if (!bExists) {
        r += "-I INPUT -j " WS_PRODUCT_NAME_LOWER "_input" + c;
        // See buildRulesV4: keep our OUTPUT jump directly below the dns-leak-protect jump when it is
        // already installed for this family, so the v6 dnsleaks DROPs are not shadowed on a late
        // firewall enable. dns-leak-protect installs a windscribe_dnsleaks jump for v6 as well.
        r += outputJumpRule(true, !iface.empty(), c);
    }

    // Loopback addresses to the local host
    r += "-A " WS_PRODUCT_NAME_LOWER "_input -s ::1 -j ACCEPT" + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_output -d ::1 -j ACCEPT" + c;

    // Always allow IPv6 link-local (required for neighbor discovery, etc.)
    r += "-A " WS_PRODUCT_NAME_LOWER "_input -s fe80::/10 -j ACCEPT" + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_output -d fe80::/10 -j ACCEPT" + c;

    // DHCPv6 (RFC 8415): client uses port 546, server uses port 547.
    r += "-A " WS_PRODUCT_NAME_LOWER "_input -p udp --sport 547 --dport 546 -j ACCEPT" + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_output -p udp --sport 546 --dport 547 -j ACCEPT" + c;

    // ICMPv6 NDP/MLD (RFC 4890 §4.4.1): NS/NA/RS/RA/Redirect (133-137) and MLDv1/v2 (130-132, 143).
    // Accepted on every interface and ahead of the address-based DROPs below so the DROP branch /
    // fc00::/7 carve-out can't break neighbor resolution or SLAAC. Safe to accept globally: these are
    // link-local, hop-limit-255-constrained control messages and are not globally routable.
    for (int icmpv6Type : {130, 131, 132, 133, 134, 135, 136, 137, 143}) {
        const std::string t = std::to_string(icmpv6Type);
        r += "-A " WS_PRODUCT_NAME_LOWER "_input -p ipv6-icmp --icmpv6-type " + t + " -j ACCEPT" + c;
        r += "-A " WS_PRODUCT_NAME_LOWER "_output -p ipv6-icmp --icmpv6-type " + t + " -j ACCEPT" + c;
    }

    // ICMPv6 connectivity-critical error messages (RFC 4890 §4.3.1): Destination Unreachable (1),
    // Packet Too Big (2), Time Exceeded (3), Parameter Problem (4). Scoped to the VPN interface, and
    // only when it carries v6: a Packet-Too-Big from a (server-controlled, possibly ULA) in-tunnel hop
    // must survive the fc00::/7 tunnel/generic DROPs below so IPv6 PMTUD doesn't blackhole. Unlike
    // NDP/MLD these types ARE globally routable and carry up to ~1200 bytes of the triggering packet,
    // so accepting them on the physical interface would punch a covert-channel / presence-disclosure
    // hole through the kill switch — hence the -i/-o iface scope. On the tunnel they'd be covered by
    // the -i/-o iface ACCEPT below anyway; this only moves them ahead of the DROPs.
    if (!iface.empty() && hasV6Addr) {
        for (int icmpv6Type : {1, 2, 3, 4}) {
            const std::string t = std::to_string(icmpv6Type);
            r += "-A " WS_PRODUCT_NAME_LOWER "_input -i " + iface + " -p ipv6-icmp --icmpv6-type " + t + " -j ACCEPT" + c;
            r += "-A " WS_PRODUCT_NAME_LOWER "_output -o " + iface + " -p ipv6-icmp --icmpv6-type " + t + " -j ACCEPT" + c;
        }
    }

    const std::string actionV6 = (config.allowLanTraffic || config.isCustomConfig) ? "ACCEPT" : "DROP";

    // Disallow ULA and application-layer multicast LAN traffic from going into the tunnel, mirroring
    // the v4 RFC1918 (fc00::/7 ~ 192.168/10/172.16) and 224.0.0.0/4 (ff00::/8) tunnel carve-outs.
    // Only emitted when allowLanTraffic makes actionV6 == ACCEPT: the carve-out must precede those
    // generic ACCEPTs below, otherwise allowLanTraffic would also open these ranges *into* the tunnel
    // whenever routing sends an off-link ULA / a multicast packet (e.g. mDNS to ff02::fb from an
    // all-interfaces avahi) to the default route. With LAN off the generic fc00::/7 + ff00::/8 DROPs
    // below already cover the tunnel, so the scoped pair would be a verdict-equivalent no-op — gating
    // on allowLanTraffic keeps the ruleset free of dead, packet-shadowing rules. (Unlike v4, whose
    // unconditional tunnel ACCEPT immediately follows its carve-out and so requires it in every
    // state.) NDP/MLD survive via the ICMPv6 carve-out above, so only non-ICMP multicast lands here.
    if (!iface.empty() && !config.isCustomConfig && config.allowLanTraffic) {
        r += "-A " WS_PRODUCT_NAME_LOWER "_input -i " + iface + " -s fc00::/7 -j DROP" + c;
        r += "-A " WS_PRODUCT_NAME_LOWER "_output -o " + iface + " -d fc00::/7 -j DROP" + c;
        r += "-A " WS_PRODUCT_NAME_LOWER "_input -i " + iface + " -s ff00::/8 -j DROP" + c;
        r += "-A " WS_PRODUCT_NAME_LOWER "_output -o " + iface + " -d ff00::/8 -j DROP" + c;
    }

    // IPv6 application-layer multicast (mDNS, SSDP, LLMNR, etc.) per allowLanTraffic / custom config.
    r += "-A " WS_PRODUCT_NAME_LOWER "_input -s ff00::/8 -j " + actionV6 + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_output -d ff00::/8 -j " + actionV6 + c;

    // IPv6 unique local addresses (RFC 4193) — the v6 analog of the RFC1918 private ranges, used
    // by typical LAN/VM-bridge setups. Gated on allowLanTraffic the same way as the v4 ranges.
    r += "-A " WS_PRODUCT_NAME_LOWER "_input -s fc00::/7 -j " + actionV6 + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_output -d fc00::/7 -j " + actionV6 + c;

    // Conditional VPN-adapter permit. Only enabled when a non-link-local v6 address is actually
    // present on the tunnel — gated to avoid leaks for v4-only tunnels.
    if (!iface.empty() && hasV6Addr) {
        spdlog::info("IPv6 permitted on VPN adapter: {}", iface);
        r += "-A " WS_PRODUCT_NAME_LOWER "_input -i " + iface + " -j ACCEPT" + c;
        r += "-A " WS_PRODUCT_NAME_LOWER "_output -o " + iface + " -j ACCEPT" + c;
    }

    r += "-A " WS_PRODUCT_NAME_LOWER "_input -j DROP" + c;
    r += "-A " WS_PRODUCT_NAME_LOWER "_output -j DROP" + c;
    r += "COMMIT\n";
    return r;
}

bool FirewallController::enabled(const std::string &tag)
{
    // Kill-switch is considered enabled if the INPUT jump into windscribe_input
    // exists in *either* family. The engine always installs v4 and v6 together,
    // but enable(ipv6=…) is invoked as two separate helper commands — checking
    // only one family used to make the second call's post-restore
    // setSplitTunnelIpExceptions() take the tear-down branch if the per-family
    // order were ever swapped. Symmetric probe removes that footgun and matches
    // the engine-level "kill-switch is on" semantic for external callers
    // (checkFirewallState).
    return inputJumpExists(false, tag) || inputJumpExists(true, tag);
}

bool FirewallController::inputJumpExists(bool isV6, const std::string &tag)
{
    const char *tool = isV6 ? "ip6tables" : "iptables";
    return Utils::executeCommand(tool, {"--check", "INPUT", "-j", WS_PRODUCT_NAME_LOWER "_input", "-m", "comment", "--comment", tag.c_str()}) == 0;
}

bool FirewallController::disable()
{
    // Tear down our own chains and the jumps into the built-in INPUT/OUTPUT chains directly,
    // rather than via a client-supplied delete blob, so firewallOff cannot be steered. Jumps must
    // be removed before the referenced chain can be deleted. iptables -D/-F/-X are no-ops (non-zero
    // exit, ignored) when the rule/chain is already absent, which gives idempotent teardown.
    for (const char *tool : {"iptables", "ip6tables"}) {
        const bool isV6 = (std::strcmp(tool, "ip6tables") == 0);

        Utils::executeCommand(tool, {"-D", "INPUT", "-j", WS_PRODUCT_NAME_LOWER "_input", "-m", "comment", "--comment", kTag.c_str()});
        Utils::executeCommand(tool, {"-D", "OUTPUT", "-j", WS_PRODUCT_NAME_LOWER "_output", "-m", "comment", "--comment", kTag.c_str()});
        if (!isV6) {
            Utils::executeCommand(tool, {"-D", "INPUT", "-j", WS_PRODUCT_NAME_LOWER "_block", "-m", "comment", "--comment", kTag.c_str()});
            Utils::executeCommand(tool, {"-D", "OUTPUT", "-j", WS_PRODUCT_NAME_LOWER "_block", "-m", "comment", "--comment", kTag.c_str()});
        }

        Utils::executeCommand(tool, {"-F", WS_PRODUCT_NAME_LOWER "_input"});
        Utils::executeCommand(tool, {"-F", WS_PRODUCT_NAME_LOWER "_output"});
        Utils::executeCommand(tool, {"-X", WS_PRODUCT_NAME_LOWER "_input"});
        Utils::executeCommand(tool, {"-X", WS_PRODUCT_NAME_LOWER "_output"});
        if (!isV6) {
            Utils::executeCommand(tool, {"-F", WS_PRODUCT_NAME_LOWER "_block"});
            Utils::executeCommand(tool, {"-X", WS_PRODUCT_NAME_LOWER "_block"});
        }
    }

    std::error_code ec;
    std::filesystem::remove(WS_LINUX_RUN_DIR "/rules.v4", ec);
    if (ec) {
        spdlog::warn("Failed to remove rules.v4: {}", ec.message());
    }
    std::filesystem::remove(WS_LINUX_RUN_DIR "/rules.v6", ec);
    if (ec) {
        spdlog::warn("Failed to remove rules.v6: {}", ec.message());
    }

    // The -D/-F/-X commands above return non-zero for already-absent rules, so their exit codes
    // can't distinguish "nothing to do" from "teardown failed". Verify the post-state instead: if a
    // jump survived (e.g. xtables lock contention), report failure so firewallOff retries rather
    // than latching the baseline to "off" while the kill switch is still up.
    return !enabled();
}

void FirewallController::probeInterfaceAddresses(const std::string &iface, std::vector<std::string> &v4Addrs, bool &hasNonLinkLocalV6)
{
    v4Addrs.clear();
    hasNonLinkLocalV6 = false;

    if (iface.empty()) {
        return;
    }

    struct ifaddrs *ifap = nullptr;
    if (getifaddrs(&ifap)) {
        spdlog::error("probeInterfaceAddresses: getifaddrs failed for {}", iface);
        return;
    }

    for (struct ifaddrs *cur = ifap; cur != nullptr; cur = cur->ifa_next) {
        if (cur->ifa_addr == nullptr || cur->ifa_name == nullptr) {
            continue;
        }
        // Exact name match: a prefix match would pull "wg01"'s addresses into a lookup for "wg0",
        // emitting another interface's local address into this adapter's ACCEPT rules.
        if (strcmp(cur->ifa_name, iface.c_str()) != 0) {
            continue;
        }
        if (cur->ifa_addr->sa_family == AF_INET) {
            char str[INET_ADDRSTRLEN] = {};
            inet_ntop(AF_INET, &(((struct sockaddr_in *)cur->ifa_addr)->sin_addr), str, sizeof(str));
            v4Addrs.push_back(str);
        } else if (cur->ifa_addr->sa_family == AF_INET6) {
            const struct sockaddr_in6 *sin6 = reinterpret_cast<const struct sockaddr_in6 *>(cur->ifa_addr);
            // Skip link-local (fe80::/10): the kernel autoconfigures one on every v6-capable iface.
            // Only a globally-scoped or ULA address indicates real v6 capability.
            if (!IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
                hasNonLinkLocalV6 = true;
            }
        }
    }

    freeifaddrs(ifap);
}

std::string FirewallController::getHotspotAdapter()
{
#ifndef CLI_ONLY
    std::string output;
    Utils::executeCommand("nmcli", {"d"}, &output);

    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("Hotspot") == std::string::npos) {
            continue;
        }
        // Expected format: "<device> <type> <state> <connection>". Only CONNECTION can contain
        // spaces, so require at least 4 fields and read device/state by their leading positions.
        std::istringstream ls(line);
        std::vector<std::string> fields;
        std::string field;
        while (ls >> field) {
            fields.push_back(field);
        }
        if (fields.size() < 4) {
            spdlog::error("Can't parse hotspot adapter name from: {}", line);
            return std::string();
        }
        if (fields[2] == "connected" || fields[2] == "Connected") {
            // Validate before interpolating into iptables rule text, same as the client-supplied
            // vpnInterfaceName: a name carrying the iptables wildcard '+' or other rule-grammar
            // characters must not reach the `-i <hotspot>` rule body.
            if (!Validation::isValidInterfaceName(fields[0])) {
                spdlog::error("getHotspotAdapter: invalid adapter name \"{}\", ignoring", fields[0]);
                return std::string();
            }
            return fields[0];
        }
    }
#endif
    return std::string();
}

void FirewallController::setSplitTunnelingEnabled(bool isConnected, bool isEnabled, bool isExclude,
                                                  const std::string &defaultAdapter,
                                                  const std::string &defaultAdapterIp,
                                                  const std::string &defaultAdapterIpV6)
{
    connected_ = isConnected;
    splitTunnelEnabled_ = isEnabled;
    splitTunnelExclude_ = isExclude;
    prevAdapter_ = defaultAdapter_;
    defaultAdapter_ = defaultAdapter;
    defaultAdapterIp_ = defaultAdapterIp;
    defaultAdapterIpV6_ = defaultAdapterIpV6;

    setSplitTunnelIpExceptions(splitTunnelIps_);
    setSplitTunnelAppExceptions();
    setSplitTunnelIngressRules(defaultAdapterIp_, defaultAdapterIpV6_);
}

void FirewallController::removeExclusiveIpRules()
{
    for (const auto &ip : splitTunnelIps_) {
        if (!ip.isValid()) {
            continue;
        }
        const char *tool = ip.isV6() ? "ip6tables" : "iptables";
        const std::string ipStr = ip.toString();
        Utils::executeCommand(tool, {"-D", WS_PRODUCT_NAME_LOWER "_input", "-s", ipStr.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
        Utils::executeCommand(tool, {"-D", WS_PRODUCT_NAME_LOWER "_output", "-d", ipStr.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
    }
}

void FirewallController::removeInclusiveIpRules()
{
    // The unconditional ACCEPT rule is added (appended) to both v4 and v6
    // windscribe_input/_output chains in setSplitTunnelAppExceptions /
    // setSplitTunnelIpExceptions, so the cleanup path has to walk both families.
    //
    // Delete by rule-spec rather than by chain position: it doesn't depend on
    // parsing `iptables -S` output and survives reordering inside the chain.
    // iptables -D is a no-op (non-zero exit) when the rule isn't present, which
    // matches the desired idempotent semantics here.
    for (const char *tool : {"iptables", "ip6tables"}) {
        Utils::executeCommand(tool, {"-D", WS_PRODUCT_NAME_LOWER "_input",  "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
        Utils::executeCommand(tool, {"-D", WS_PRODUCT_NAME_LOWER "_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
    }
}

void FirewallController::removeExclusiveAppRules()
{
    Utils::executeCommand("iptables", {"-D", "OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
    Utils::executeCommand("ip6tables", {"-D", "OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
    if (!prevAdapter_.empty()) {
        Utils::executeCommand("iptables", {"-D", "POSTROUTING", "-t", "nat", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-o", prevAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
        Utils::executeCommand("ip6tables", {"-D", "POSTROUTING", "-t", "nat", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-o", prevAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
    }
}

void FirewallController::removeInclusiveAppRules()
{
    Utils::executeCommand("iptables", {"-D", "OUTPUT", "-t", "mangle", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
    Utils::executeCommand("ip6tables", {"-D", "OUTPUT", "-t", "mangle", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
    if (!prevAdapter_.empty()) {
        Utils::executeCommand("iptables", {"-D", "POSTROUTING", "-t", "nat", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-o", prevAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
        Utils::executeCommand("ip6tables", {"-D", "POSTROUTING", "-t", "nat", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-o", prevAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
    }
}

void FirewallController::setSplitTunnelIngressRules(const std::string &defaultAdapterIp, const std::string &defaultAdapterIpV6)
{
    if (defaultAdapterIp.empty() && defaultAdapterIpV6.empty()) {
        return;
    }

    if (!connected_ || !splitTunnelEnabled_ || splitTunnelExclude_) {
        if (!defaultAdapterIp.empty()) {
            Utils::executeCommand("iptables", {"-D", "PREROUTING", "-t", "mangle", "-d", defaultAdapterIp.c_str(), "-j", "CONNMARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
            Utils::executeCommand("iptables", {"-D", "OUTPUT", "-t", "mangle", "-j", "CONNMARK", "--restore-mark", "-m", "comment", "--comment", kTag});
        }
        if (!defaultAdapterIpV6.empty()) {
            Utils::executeCommand("ip6tables", {"-D", "PREROUTING", "-t", "mangle", "-d", defaultAdapterIpV6.c_str(), "-j", "CONNMARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
            Utils::executeCommand("ip6tables", {"-D", "OUTPUT", "-t", "mangle", "-j", "CONNMARK", "--restore-mark", "-m", "comment", "--comment", kTag});
        }
        return;
    }

    if (!defaultAdapterIp.empty()) {
        addRule({"PREROUTING", "-t", "mangle", "-d", defaultAdapterIp.c_str(), "-j", "CONNMARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, true);
        addRule({"OUTPUT", "-t", "mangle", "-j", "CONNMARK", "--restore-mark", "-m", "comment", "--comment", kTag}, true);
    }
    if (!defaultAdapterIpV6.empty()) {
        // connmark itself is family-agnostic at the kernel level, but the per-family
        // PREROUTING -d match has to live in the matching ip6tables table so v6
        // ingress on the physical adapter sets the mark.
        addRule({"PREROUTING", "-t", "mangle", "-d", defaultAdapterIpV6.c_str(), "-j", "CONNMARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, true, /*isV6=*/true);
        addRule({"OUTPUT", "-t", "mangle", "-j", "CONNMARK", "--restore-mark", "-m", "comment", "--comment", kTag}, true, /*isV6=*/true);
    }
}

void FirewallController::setSplitTunnelAppExceptions()
{
    if (!connected_ || !splitTunnelEnabled_) {
        removeExclusiveAppRules();
        removeInclusiveAppRules();
        return;
    }

    if (splitTunnelExclude_) {
        removeInclusiveAppRules();

        // The cgroup match works for both v4 and v6 packets (single net_cls cgroup),
        // so MARK/MASQUERADE/ACCEPT mirror unchanged into ip6tables. MASQUERADE
        // requires nf_nat_ipv6 (built into nf_nat on modern kernels); on hosts with
        // no v6 connectivity the v6 rule is a harmless no-op.
        addRule({"POSTROUTING",  "-t", "nat", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-o", defaultAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag}, true);
        addRule({"POSTROUTING",  "-t", "nat", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-o", defaultAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag}, true, /*isV6=*/true);
        addRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, false);
        addRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);

        // allow packets from excluded apps, if firewall is on
        if (enabled()) {
            addRule({WS_PRODUCT_NAME_LOWER "_input", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            addRule({WS_PRODUCT_NAME_LOWER "_output", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            addRule({WS_PRODUCT_NAME_LOWER "_input", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);
            addRule({WS_PRODUCT_NAME_LOWER "_output", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);
        }
    } else {
        removeExclusiveAppRules();

        addRule({"POSTROUTING", "-t", "nat", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-o", defaultAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag}, true);
        addRule({"POSTROUTING", "-t", "nat", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-o", defaultAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag}, true, /*isV6=*/true);
        addRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, false);
        addRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);

        // For inclusive, allow all packets
        if (enabled()) {
            addRule({WS_PRODUCT_NAME_LOWER "_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            addRule({WS_PRODUCT_NAME_LOWER "_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            addRule({WS_PRODUCT_NAME_LOWER "_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);
            addRule({WS_PRODUCT_NAME_LOWER "_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);
        }
    }
}

void FirewallController::setSplitTunnelIpExceptions(const std::vector<types::IpAddressRange> &ips)
{
    if (!connected_ || !splitTunnelEnabled_ || !enabled()) {
        removeInclusiveIpRules();
        removeExclusiveIpRules();
        splitTunnelIps_ = ips;
        return;
    }

    // Otherwise, split tunneling is still enabled.
    if (splitTunnelExclude_) {
        removeInclusiveIpRules();

        // For exclusive, remove rules for addresses no longer in "ips" (per-family).
        for (const auto &ip : splitTunnelIps_) {
            if (!ip.isValid()) {
                continue;
            }
            if (std::find(ips.begin(), ips.end(), ip) == ips.end()) {
                const char *tool = ip.isV6() ? "ip6tables" : "iptables";
                const std::string ipStr = ip.toString();
                Utils::executeCommand(tool, {"-D", WS_PRODUCT_NAME_LOWER "_input", "-s", ipStr.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
                Utils::executeCommand(tool, {"-D", WS_PRODUCT_NAME_LOWER "_output", "-d", ipStr.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            }
        }

        // Add rules for IPs in the new list, dispatching to iptables / ip6tables
        // by family. Invalid entries (shouldn't happen — caller filters) are skipped.
        for (const auto &ip : ips) {
            if (!ip.isValid()) {
                continue;
            }
            const std::string ipStr = ip.toString();
            const bool isV6 = ip.isV6();
            addRule({WS_PRODUCT_NAME_LOWER "_input", "-s", ipStr.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, isV6);
            addRule({WS_PRODUCT_NAME_LOWER "_output", "-d", ipStr.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, isV6);
        }
    } else {
        removeExclusiveIpRules();

        // For inclusive, keep the "allow all" rules; these rules only apply to non-included apps.
        // Mirror to ip6tables so v6 traffic on the windscribe_input/_output chains is also let
        // through for non-included flows.
        addRule({WS_PRODUCT_NAME_LOWER "_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
        addRule({WS_PRODUCT_NAME_LOWER "_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
        addRule({WS_PRODUCT_NAME_LOWER "_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);
        addRule({WS_PRODUCT_NAME_LOWER "_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);
    }

    splitTunnelIps_ = ips;
}

void FirewallController::addRule(const std::vector<std::string> &args, bool prepend, bool isV6)
{
    const char *tool = isV6 ? "ip6tables" : "iptables";
    std::vector<std::string> checkArgs = args;
    checkArgs.insert(checkArgs.begin(), "-C");
    int ret = Utils::executeCommand(tool, checkArgs);
    if (ret) {
        std::vector<std::string> insertArgs = args;
        if (prepend) {
            insertArgs.insert(insertArgs.begin(), "-I");
        } else {
            insertArgs.insert(insertArgs.begin(), "-A");
        }
        Utils::executeCommand(tool, insertArgs);
    }
}
