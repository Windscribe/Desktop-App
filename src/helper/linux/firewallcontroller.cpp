#include "firewallcontroller.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sstream>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "../common/validation_posix.h"
#include "network_marks.h"
#include "nftables/nftablescontroller.h"
#include "split_tunneling/cgroups.h"
#include "utils.h"

FirewallController::FirewallController() : connected_(false), splitTunnelEnabled_(false), splitTunnelExclude_(true)
{
    // Remove any pre-nftables iptables state left by an older install before we start managing the
    // nft table, so the two can't both be live.
    purgeLegacyIptables();
}

FirewallController::~FirewallController()
{
}

std::string FirewallController::buildFirewallRules(const FirewallConfig &config,
                                                     const std::vector<std::string> &localV4Addrs,
                                                     bool hasV6Addr, bool &ok)
{
    ok = true;
    const std::string &iface = config.vpnInterfaceName;
    const std::string hotspot = getHotspotAdapter();
    const bool custom = config.isCustomConfig;

    std::string r;
    auto in  = [&](const std::string &e) { r += nft::rule("input", e); };
    auto out = [&](const std::string &e) { r += nft::rule("output", e); };

    // --- Loopback + DHCPv4 (v6 uses different ports below) ---
    in("iifname \"lo\" accept");
    out("oifname \"lo\" accept");
    // Scoped to ipv4 — in an inet table an L4-only match would also accept IPv6 on this port pair,
    // widening the firewall beyond v4 DHCP.
    in("meta nfproto ipv4 udp sport 67-68 udp dport 67-68 accept");
    out("meta nfproto ipv4 udp sport 67-68 udp dport 67-68 accept");

    // --- IPv4 ---
    if (!iface.empty()) {
        if (!custom) {
            // Allow the tunnel's own local addresses.
            for (const auto &addr : localV4Addrs) {
                in("iifname \"" + iface + "\" ip saddr " + addr + "/32 accept");
                out("oifname \"" + iface + "\" ip daddr " + addr + "/32 accept");
            }
            // Disallow LAN (except 10.255.255.0/24), link-local, and multicast from entering the tunnel.
            in("iifname \"" + iface + "\" ip saddr 192.168.0.0/16 drop");
            out("oifname \"" + iface + "\" ip daddr 192.168.0.0/16 drop");
            in("iifname \"" + iface + "\" ip saddr 172.16.0.0/12 drop");
            out("oifname \"" + iface + "\" ip daddr 172.16.0.0/12 drop");
            in("iifname \"" + iface + "\" ip saddr 169.254.0.0/16 drop");
            out("oifname \"" + iface + "\" ip daddr 169.254.0.0/16 drop");
            in("iifname \"" + iface + "\" ip saddr 10.255.255.0/24 accept");
            out("oifname \"" + iface + "\" ip daddr 10.255.255.0/24 accept");
            in("iifname \"" + iface + "\" ip saddr 10.0.0.0/8 drop");
            out("oifname \"" + iface + "\" ip daddr 10.0.0.0/8 drop");
            in("iifname \"" + iface + "\" ip saddr 224.0.0.0/4 drop");
            out("oifname \"" + iface + "\" ip daddr 224.0.0.0/4 drop");
        }

        in("iifname \"" + iface + "\" meta nfproto ipv4 accept");
        out("oifname \"" + iface + "\" meta nfproto ipv4 accept");

        if (!hotspot.empty()) {
            in("iifname \"" + hotspot + "\" meta nfproto ipv4 accept");
            out("oifname \"" + hotspot + "\" meta nfproto ipv4 accept");
        }
    }

    if (!config.connectingIp.empty()) {
        const std::string &ip = config.connectingIp;
        in("ip saddr " + ip + "/32 accept");
        // Allow packets from gid 0
        out("ip daddr " + ip + "/32 meta skgid 0 accept");
        // Allow the obfuscation proxies (stunnel/wstunnel) to reach the endpoint. The helper launches
        // them as the windscribe service user, so they are matched on that uid — not the windscribe
        // group. The client binary is setgid (not setuid) windscribe, so a process that merely carries
        // the group (the GUI, or code injected into it via e.g. a tainted GL/EGL driver) has euid != the
        // service user and is correctly excluded; only the helper's own setuid'd proxies match. Root
        // daemons (openvpn, wireguard-go) are covered by the gid-0 rule above, and the kernel WireGuard
        // module by the no-uid rule below.
        const struct passwd *pw = getpwnam(WS_PRODUCT_NAME_LOWER);
        if (pw != nullptr) {
            out("ip daddr " + ip + "/32 meta skuid " + std::to_string(pw->pw_uid) + " accept");
        } else {
            // Without this rule the proxies' packets hit the policy-drop output chain and obfuscated
            // protocols can't connect. Fail the apply so enable() returns false and the engine retries,
            // rather than silently committing a degraded ruleset.
            spdlog::error("buildFirewallRules: getpwnam(\"" WS_PRODUCT_NAME_LOWER "\") failed; aborting firewall apply so it is retried");
            ok = false;
        }
        // Allow packets from kernel (no uid), for wireguard control traffic
        out("ip daddr " + ip + "/32 meta skuid != 0-4294967294 accept");
        // Allow marked packets.  These packets are marked by the wireguard adapter code.
        // This is necessary because wg packets have uid/gid of the app that created the packet, not wg itself.
        out("ip daddr " + ip + "/32 meta mark " + std::to_string(marks::kWireGuardFwMark) + " accept");
        out("ip daddr " + ip + "/32 meta mark " + std::to_string(marks::kOpenVpnFwMark) + " accept");
    }

    for (const auto &i : config.allowedIps) {
        in("ip saddr " + i + "/32 accept");
        out("ip daddr " + i + "/32 accept");
    }

    // Drop the hotspot adapter in the disconnected state (the accept above wins when connected).
    if (!hotspot.empty()) {
        in("iifname \"" + hotspot + "\" meta nfproto ipv4 drop");
        out("oifname \"" + hotspot + "\" meta nfproto ipv4 drop");
    }

    // Loopback addresses to the local host.
    in("ip saddr 127.0.0.0/8 accept");
    out("ip daddr 127.0.0.0/8 accept");

    // Local network + multicast (v4). Custom configs need private ranges allowed so third-party VPN
    // DNS/gateway/routes work. Shared with the boot ruleset via nft::lanRulesV4.
    const std::string act = (config.allowLanTraffic || custom) ? "accept" : "drop";
    r += nft::lanRulesV4(act);

    // --- IPv6 ---
    // Loopback to the local host.
    in("ip6 saddr ::1 accept");
    out("ip6 daddr ::1 accept");

    // Always allow IPv6 link-local (neighbor discovery, etc.).
    in("ip6 saddr fe80::/10 accept");
    out("ip6 daddr fe80::/10 accept");

    // DHCPv6 (RFC 8415): client port 546, server port 547. Scoped to ipv6 — in an inet table an
    // L4-only match would also accept IPv4 on this port pair, widening the firewall beyond v6 DHCP.
    in("meta nfproto ipv6 udp sport 547 udp dport 546 accept");
    out("meta nfproto ipv6 udp sport 546 udp dport 547 accept");

    // ICMPv6 NDP/MLD (RFC 4890 §4.4.1): NS/NA/RS/RA/Redirect (133-137) and MLDv1/v2 (130-132, 143).
    // Accepted on every interface and ahead of the address-based drops so neighbor resolution / SLAAC
    // are never broken. Safe globally: these are link-local, hop-limit-255 control messages.
    in("icmpv6 type { 130, 131, 132, 133, 134, 135, 136, 137, 143 } accept");
    out("icmpv6 type { 130, 131, 132, 133, 134, 135, 136, 137, 143 } accept");

    // ICMPv6 connectivity-critical error messages (RFC 4890 §4.3.1): Destination Unreachable (1),
    // Packet Too Big (2), Time Exceeded (3), Parameter Problem (4). Scoped to the VPN interface and
    // only when it carries v6, so a Packet-Too-Big from an in-tunnel hop survives the fc00::/7 drops
    // (PMTUD) without punching a covert channel through the firewall on the physical interface.
    if (!iface.empty() && hasV6Addr) {
        in("iifname \"" + iface + "\" icmpv6 type { 1, 2, 3, 4 } accept");
        out("oifname \"" + iface + "\" icmpv6 type { 1, 2, 3, 4 } accept");
    }

    const std::string actV6 = (config.allowLanTraffic || custom) ? "accept" : "drop";

    // Disallow ULA and multicast LAN traffic from entering the tunnel (mirrors the v4 carve-outs).
    // Only emitted when allowLanTraffic makes the generic ACCEPTs below open these ranges, otherwise
    // the generic fc00::/7 + ff00::/8 drops already cover the tunnel and these would be dead rules.
    if (!iface.empty() && !custom && config.allowLanTraffic) {
        in("iifname \"" + iface + "\" ip6 saddr fc00::/7 drop");
        out("oifname \"" + iface + "\" ip6 daddr fc00::/7 drop");
        in("iifname \"" + iface + "\" ip6 saddr ff00::/8 drop");
        out("oifname \"" + iface + "\" ip6 daddr ff00::/8 drop");
    }

    // IPv6 multicast (ff00::/8) + unique-local addresses (fc00::/7) per allowLanTraffic / custom config.
    // Shared with the boot ruleset via nft::lanRulesV6.
    r += nft::lanRulesV6(actV6);

    // Conditional VPN-adapter permit, only when a non-link-local v6 address is present on the tunnel
    // (gated to avoid leaks on v4-only tunnels).
    if (!iface.empty() && hasV6Addr) {
        spdlog::info("IPv6 permitted on VPN adapter: {}", iface);
        in("iifname \"" + iface + "\" meta nfproto ipv6 accept");
        out("oifname \"" + iface + "\" meta nfproto ipv6 accept");
    }

    return r;
}

bool FirewallController::enable(const FirewallConfig &config)
{
    // Locked: enable() reads the split-tunnel state (buildSplitTunnelRules) that the resolver-thread
    // applySplitTunnel() can be mutating concurrently.
    std::lock_guard<std::recursive_mutex> lock(stateMutex_);

    // Probe the VPN adapter once (single getifaddrs walk): the v4 builder needs its local v4
    // addresses, the v6 builder needs only whether a non-link-local v6 address is present.
    std::vector<std::string> localV4Addrs;
    bool hasV6Addr = false;
    if (!config.vpnInterfaceName.empty()) {
        probeInterfaceAddresses(config.vpnInterfaceName, localV4Addrs, hasV6Addr);
    }


    std::string buf = nft::addTable();
    // Build the split-tunnel chains first, in the same transaction: it creates st_in/st_out so the
    // jumps from input/output below resolve, and folding it in keeps the firewall and split-tunnel
    // state consistent with one apply instead of a second nft round-trip.
    buf += buildSplitTunnelRules();
    // Create our firewall base chains if absent, then flush their rules (see nft::ensureChain for why a
    // base chain is never delete+re-added). This only touches input/output — the dnsleaks/wg/boot
    // chains are untouched.
    buf += nft::ensureChain("input", kInputChainSpec);
    buf += nft::ensureChain("output", kOutputChainSpec);
    bool rulesOk = true;
    buf += buildFirewallRules(config, localV4Addrs, hasV6Addr, rulesOk);
    if (!rulesOk) {
        // A required rule could not be built (e.g. the windscribe group is unresolvable). Don't commit
        // a partial ruleset that would block the connection; return false so the engine retries.
        return false;
    }
    // Excluded-app / per-IP split-tunnel exceptions must bypass the terminal drop, so jump last.
    buf += nft::rule("input", "jump st_in");
    buf += nft::rule("output", "jump st_out");

    // Commit directly. An nft transaction is atomic, so a rejected ruleset fails the commit as a unit
    // and leaves the existing rules intact — no separate dry-run validation is needed (and the
    // in-process dry-run pass triggers a spurious EBUSY on re-apply).
    return NftablesController::instance().run(buf);
}

bool FirewallController::disable()
{
    // Tear down all of our firewall and split-tunnel chains, leaving only the dnsleaks / wg_* / boot
    // chains and the table itself intact. The split-tunnel routing/NAT/connmark base chains
    // (st_route_out, st_nat_post, st_pre_cm) are removed too: leaving them behind made the table linger
    // after disconnect (so the UI still read the firewall as on) and meant the next apply tried to
    // re-add already-installed hooked base chains, which the kernel rejects with EOPNOTSUPP. Removing
    // them here lets each split-tunnel activation re-create them cleanly. nft::deleteChain (add-then-
    // delete, no hook spec) makes each removal idempotent.
    //
    // This MUST be two transactions, not one. st_in/st_out are jump targets of the `jump st_in` /
    // `jump st_out` rules in the input/output base chains. The kernel refuses to delete a chain while
    // anything still references it (EBUSY) and counts that reference for the whole transaction — even
    // when the referencing chain is itself being deleted in the same batch. Folding the jump-target
    // deletes into the same transaction would fail as a unit, leaving the policy-drop input/output
    // chains installed: the firewall never turns off on disconnect. So first delete input/output (which
    // removes the jump rules and releases the references) along with the non-referenced route/NAT/
    // connmark chains, then delete the now-unreferenced st_in/st_out.
    std::string buf = nft::addTable();
    buf += nft::deleteChain("input");
    buf += nft::deleteChain("output");
    buf += nft::deleteChain("st_route_out");
    buf += nft::deleteChain("st_nat_post");
    buf += nft::deleteChain("st_pre_cm");
    // The dnsleaks chain is owned by the DNS-leak path and normally torn down on disconnect. When the
    // firewall is turned off while not connected, that chain has no reason to persist; remove it here as
    // a safety net so its port-53 drops can't be left blocking the OS resolvers if a disconnect teardown
    // was missed. While connected we leave it intact (DNS-leak protection is independent of the kill
    // switch). deleteChain is idempotent, so this is a no-op when the chain is already absent.
    {
        std::lock_guard<std::recursive_mutex> lock(stateMutex_);
        if (!connected_) {
            buf += nft::deleteChain("dnsleaks");
        }
    }
    // A failed run means the policy-drop chains may still be installed, so report the firewall as still
    // up and let firewallOff retry. We deliberately do NOT fall back to probing enabled() on failure: a
    // transient error in that query reads as "off" and would latch the baseline off over a still-live
    // kill-switch chain.
    if (!NftablesController::instance().run(buf)) {
        return false;
    }

    // input/output (and their jumps) are gone, so st_in/st_out are now unreferenced and can be removed.
    std::string stBuf = nft::addTable();
    stBuf += nft::deleteChain("st_in");
    stBuf += nft::deleteChain("st_out");
    return NftablesController::instance().run(stBuf);
}

bool FirewallController::enabled()
{
    // The firewall is on iff our output base chain exists. A missing chain is the normal "off" result,
    // but any OTHER nft failure (transient resource/parse error) must not be silently read as "off":
    // firewallActualState() gates whether the engine re-applies the kill switch, so a spurious "off"
    // could suppress a needed re-apply or mask a still-installed policy-drop chain. On such an error we
    // retry the probe once; only a definitive "chain absent" result is reported as off.
    const std::string query = "list chain " + kNftTable + " output";
    for (int attempt = 0; attempt < 2; ++attempt) {
        std::string err;
        if (NftablesController::instance().run(query, /*dryRun=*/false, &err, /*logOnError=*/false)) {
            return true;
        }
        if (err.empty() || err.find("No such") != std::string::npos) {
            return false;
        }
        spdlog::warn("FirewallController::enabled: unexpected nft error querying firewall state (attempt {}): {}", attempt + 1, err);
    }
    // Two consecutive unexpected errors: we cannot confirm the chain is gone, and nft itself appears
    // broken (so enabling would fail too). Report the firewall as still on rather than risk tearing
    // down or skipping a re-apply of a kill switch that may be live.
    return true;
}

std::string FirewallController::buildSplitTunnelRules()
{
    const std::string classid = CGroups::instance().netClassId();
    const std::string mark = CGroups::instance().mark();

    std::string buf;
    // st_in/st_out are the firewall's jump targets (input/output unconditionally `jump` them), so they
    // must exist whenever the firewall is up. They are regular chains (no hook), so re-issuing the
    // create is a true idempotent no-op.
    buf += nft::ensureChain("st_in");
    buf += nft::ensureChain("st_out");

    if (connected_ && splitTunnelEnabled_) {
        // The routing/NAT/connmark base chains are only needed while split tunneling is active, so
        // create them HERE rather than on every firewall apply. Re-issuing `add chain { type … hook … }`
        // for an already-installed *hooked* base chain is rejected by the kernel (EOPNOTSUPP) — and if a
        // prior build created it at a different priority, the conflicting redefinition fails the same
        // way — which would otherwise fail every enable()/apply once these chains existed. disable()
        // tears them down so each activation re-creates them cleanly. st_pre_cm sits at prerouting
        // priority -140, deterministically AFTER WireGuardAdapter's wg_mangle_pre (-150), so the two
        // connmark chains never run in kernel-undefined order on the same packets.
        buf += nft::ensureChain("st_route_out", "type route hook output priority -150;");
        buf += nft::ensureChain("st_nat_post", "type nat hook postrouting priority 100;");
        buf += nft::ensureChain("st_pre_cm", "type filter hook prerouting priority -140;");

        const bool defAdapterValid = !defaultAdapter_.empty() && Validation::isValidInterfaceName(defaultAdapter_);
        auto add = [&](const std::string &ch, const std::string &e) {
            buf += nft::rule(ch, e);
        };

        if (splitTunnelExclude_) {
            // Excluded apps (in the cgroup) bypass the tunnel: masquerade out the physical adapter and
            // mark their packets so policy routing sends them around the tunnel.
            if (defAdapterValid) {
                add("st_nat_post", "meta cgroup " + classid + " oifname \"" + defaultAdapter_ + "\" masquerade");
            }
            add("st_route_out", "meta cgroup " + classid + " meta mark set " + mark);
            // Per-IP exceptions + the cgroup accept, in the firewall jump targets so excluded
            // traffic is permitted past the drop policy.
            for (const auto &ip : splitTunnelIps_) {
                if (!ip.isValid()) {
                    continue;
                }
                const std::string s = ip.toString();
                if (ip.isV6()) {
                    add("st_in",  "ip6 saddr " + s + " accept");
                    add("st_out", "ip6 daddr " + s + " accept");
                } else {
                    add("st_in",  "ip saddr " + s + " accept");
                    add("st_out", "ip daddr " + s + " accept");
                }
            }
            add("st_in",  "meta cgroup " + classid + " accept");
            add("st_out", "meta cgroup " + classid + " accept");
        } else {
            // Inclusive: everything NOT in the cgroup bypasses the tunnel. connmark carries the mark
            // across the reply path. The egress MARK, the reply-path restore, and the ingress connmark
            // SET are all-or-nothing: marking egress without a valid physical-adapter IP to restore the
            // reply path would leave replies unmarked and mis-routed, so gate the whole set together.
            const bool hasV4 = Validation::isValidIpAddress(defaultAdapterIp_);
            const bool hasV6 = Validation::isValidIpAddress(defaultAdapterIpV6_);
            if (defAdapterValid) {
                add("st_nat_post", "meta cgroup != " + classid + " oifname \"" + defaultAdapter_ + "\" masquerade");
            }
            if (hasV4 || hasV6) {
                add("st_route_out", "meta mark set ct mark");
                add("st_route_out", "meta cgroup != " + classid + " meta mark set " + mark);
                if (hasV4) {
                    add("st_pre_cm", "ip daddr " + defaultAdapterIp_ + " ct mark set " + mark);
                }
                if (hasV6) {
                    add("st_pre_cm", "ip6 daddr " + defaultAdapterIpV6_ + " ct mark set " + mark);
                }
            }
            add("st_in",  "accept");
            add("st_out", "accept");
        }
    }

    return buf;
}

bool FirewallController::applySplitTunnel()
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex_);
    if (!NftablesController::instance().run(nft::addTable() + buildSplitTunnelRules())) {
        spdlog::error("FirewallController::applySplitTunnel: nft apply failed; split-tunnel exceptions may not be installed");
        return false;
    }
    return true;
}

void FirewallController::setSplitTunnelingEnabled(bool isConnected, bool isEnabled, bool isExclude,
                                                  const std::string &adapter,
                                                  const std::string &adapterIp,
                                                  const std::string &adapterIpV6)
{
    // Mutate and apply atomically under one lock so a concurrent applySplitTunnel() can't observe a
    // half-updated snapshot (the apply re-enters the recursive lock).
    std::lock_guard<std::recursive_mutex> lock(stateMutex_);
    connected_ = isConnected;
    splitTunnelEnabled_ = isEnabled;
    splitTunnelExclude_ = isExclude;
    defaultAdapter_ = adapter;
    defaultAdapterIp_ = adapterIp;
    defaultAdapterIpV6_ = adapterIpV6;

    applySplitTunnel();
}

bool FirewallController::setSplitTunnelIpExceptions(const std::vector<types::IpAddressRange> &ips, bool applyNow)
{
    // Stores the exception list and, by default, applies it as one nft transaction. The default applies
    // so a caller that merely sets exceptions can never silently leave them uninstalled.
    //
    // applyNow=false stores WITHOUT applying, and is valid ONLY when a setSplitTunnelingEnabled() call
    // (which rebuilds and applies the chains from full state) is guaranteed to follow — i.e. the connect
    // path, where applying here too would emit a wasted intermediate transaction built from stale connect
    // state. Do not pass false from anywhere that isn't immediately followed by setSplitTunnelingEnabled().
    std::lock_guard<std::recursive_mutex> lock(stateMutex_);
    splitTunnelIps_ = ips;
    if (applyNow) {
        return applySplitTunnel();
    }
    return true;
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
        // Exact name match: a prefix match would pull "wg01"'s addresses into a lookup for "wg0".
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
            // Validate before interpolating into rule text, same as the client-supplied
            // vpnInterfaceName: a name carrying nft grammar characters must not reach the rule body.
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

void FirewallController::purgeLegacyIptables()
{
    // One-shot cleanup of pre-nftables state from an older install: the iptables windscribe_* filter
    // chains + their INPUT/OUTPUT jumps, and the legacy windscribe_dnsleaks chain. Idempotent:
    // iptables -D/-F/-X return non-zero (ignored) when the rule/chain is already absent, so this is a
    // no-op once the legacy state is gone. iptables is no longer a dependency, so each family is
    // skipped when its binary isn't installed.
    const std::string tag = kTag;
    for (const char *tool : {"iptables", "ip6tables"}) {
        if (Utils::executeCommand(tool, {"--version"}) != 0) {
            continue;
        }
        const bool isV6 = (std::strcmp(tool, "ip6tables") == 0);

        Utils::executeCommand(tool, {"-D", "INPUT",  "-j", WS_PRODUCT_NAME_LOWER "_input",  "-m", "comment", "--comment", tag});
        Utils::executeCommand(tool, {"-D", "OUTPUT", "-j", WS_PRODUCT_NAME_LOWER "_output", "-m", "comment", "--comment", tag});
        if (!isV6) {
            Utils::executeCommand(tool, {"-D", "INPUT",  "-j", WS_PRODUCT_NAME_LOWER "_block", "-m", "comment", "--comment", tag});
            Utils::executeCommand(tool, {"-D", "OUTPUT", "-j", WS_PRODUCT_NAME_LOWER "_block", "-m", "comment", "--comment", tag});
        }
        // Legacy dns-leak chain, matched by its marker comment.
        Utils::executeCommand(tool, {"-D", "OUTPUT", "-j", WS_PRODUCT_NAME_LOWER "_dnsleaks", "-m", "comment", "--comment", "Windscribe client dns leak protection"});

        Utils::executeCommand(tool, {"-F", WS_PRODUCT_NAME_LOWER "_input"});
        Utils::executeCommand(tool, {"-F", WS_PRODUCT_NAME_LOWER "_output"});
        Utils::executeCommand(tool, {"-F", WS_PRODUCT_NAME_LOWER "_dnsleaks"});
        Utils::executeCommand(tool, {"-X", WS_PRODUCT_NAME_LOWER "_input"});
        Utils::executeCommand(tool, {"-X", WS_PRODUCT_NAME_LOWER "_output"});
        Utils::executeCommand(tool, {"-X", WS_PRODUCT_NAME_LOWER "_dnsleaks"});
        if (!isV6) {
            Utils::executeCommand(tool, {"-F", WS_PRODUCT_NAME_LOWER "_block"});
            Utils::executeCommand(tool, {"-X", WS_PRODUCT_NAME_LOWER "_block"});
        }
    }
}
