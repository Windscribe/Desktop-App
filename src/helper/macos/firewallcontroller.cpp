#include "firewallcontroller.h"

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <set>
#include <sstream>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "../common/io_posix.h"
#include "../common/validation_posix.h"
#include "macutils.h"
#include "utils.h"

namespace {

// Emits the inline pf anchor block used inside the main pf.conf. Mirrors the engine-side helper that
// previously built these rules.
std::string anchorBlock(const std::string &name, const std::vector<std::string> &rules)
{
    if (rules.empty()) {
        return "anchor " + name + " all";
    }
    std::string str = "anchor " + name + " all {\n";
    for (const auto &r : rules) {
        str += r + "\n";
    }
    str += "}";
    return str;
}

} // namespace

FirewallController::FirewallController() : enabled_(false)
{
    // If firewall on boot is enabled, restore boot rules
    if (Utils::isFileExists(WS_POSIX_CONFIG_DIR "/boot_pf.conf")) {
        spdlog::info("Applying boot pfctl rules");
        Utils::executeCommand("/sbin/pfctl", {"-e", "-f", WS_POSIX_CONFIG_DIR "/boot_pf.conf"});
    }
}

FirewallController::~FirewallController()
{
}

bool FirewallController::enable(const FirewallConfig &config)
{
    // Off->on transition does a full load (state flush is expected/acceptable there). Once we're
    // enabled, every change is applied with targeted table/anchor loads that leave the pf state
    // table untouched, so live connections aren't reset on an allowed-IP / LAN / ports / interface
    // update. This mirrors the engine's previous incremental behavior.
    if (!hasLastConfig_) {
        if (enabled() && ourRulesPresent()) {
            // Adopt a ruleset left active by a previous helper instance: we have no cached config to
            // diff against, so refresh every piece via the non-flushing targeted loads rather than a
            // state-flushing full reload.
            if (!applyTargeted(config, /*force=*/true)) {
                return false;
            }
            enabled_ = true;
            // Refresh <windscribe_dns> only when we already know the VPN DNS — i.e. an earlier
            // setVpnDns in this fresh process populated vpnDns_ but flushed the table because
            // enabled_ was still false; this reload installs it now that we're enabled. If we don't
            // know it yet (no connect-status push arrived), leave the table the previous instance
            // left in the kernel intact: it is almost certainly still correct, whereas updateDns()
            // with an empty vpnDns_ would flush it and block all VPN DNS until the next push.
            // setVpnDns() reloads it once that push arrives.
            if (!vpnDns_.empty()) {
                updateDns();
            }
        } else if (!fullApply(config)) {
            // The full load failed (couldn't write or parse/apply the ruleset). Leave hasLastConfig_
            // false so the next enable() retries a full load rather than patching anchors of a
            // ruleset that was never applied.
            return false;
        }
    } else if (!applyTargeted(config, /*force=*/false)) {
        // A targeted load failed; the active ruleset now diverges from this intent. Leave
        // lastConfig_ untouched so the next push still diffs against the last good config and
        // retries the failed piece rather than treating it as already applied.
        return false;
    }

    lastConfig_ = config;
    hasLastConfig_ = true;
    return true;
}

bool FirewallController::applyTargeted(const FirewallConfig &config, bool force)
{
    // allowedIps originates from an unordered QSet on the engine, so its vector order can vary
    // between calls — compare as a set to avoid a spurious table reload. staticPorts arrives in a
    // deterministic order, so a plain vector compare below is sufficient there.
    auto sameIpSet = [](const std::vector<std::string> &a, const std::vector<std::string> &b) {
        return std::set<std::string>(a.begin(), a.end()) == std::set<std::string>(b.begin(), b.end());
    };

    bool ok = true;
    if (force || !sameIpSet(config.allowedIps, lastConfig_.allowedIps)) {
        ok = loadTableDef(ipsTableDef(config.allowedIps)) && ok;
    }
    // lanTrafficRules' pass/block action is (allowLanTraffic || isCustomConfig), so isCustomConfig
    // flips the LAN rules just as allowLanTraffic does — gate on both. Otherwise switching to/from a
    // custom config without an allowLanTraffic change would leave the LAN anchor stale (custom
    // config's private gateway/DNS blocked, or LAN left open after switching back).
    if (force ||
        config.allowLanTraffic != lastConfig_.allowLanTraffic ||
        config.isCustomConfig != lastConfig_.isCustomConfig) {
        ok = loadAnchor(WS_PRODUCT_NAME_LOWER "_lan_traffic", lanTrafficRules(config.allowLanTraffic, config.isCustomConfig)) && ok;
    }
    // vpnTrafficRules bakes in live interface state (the adapter's local v4 addresses and whether it
    // carries a non-link-local v6 address) that is not part of FirewallConfig, so gate on the
    // generated rules rather than the config fields alone — otherwise a tunnel that gains a v6
    // address or has its v4 address reassigned without a config-field change would keep stale rules.
    // Comparing the generated text subsumes the connectingIp/isCustomConfig/vpnInterfaceName checks,
    // which all alter it. Mirrors the <disallowed_dns> diff-gate below.
    const std::vector<std::string> vpnRules = vpnTrafficRules(config);
    bool vpnReloaded = false;
    if (force || vpnRules != lastVpnTrafficRules_) {
        if (loadAnchor(WS_PRODUCT_NAME_LOWER "_vpn_traffic", vpnRules)) {
            vpnReloaded = true;
        } else {
            ok = false;
        }
    }
    if (force || config.staticPorts != lastConfig_.staticPorts) {
        ok = loadAnchor(WS_PRODUCT_NAME_LOWER "_static_ports_traffic", staticPortsRules(config.staticPorts)) && ok;
    }
    // The OS resolver set can change while the firewall stays enabled (e.g. roaming to a network
    // whose DHCP hands out new resolvers), and a push is not correlated with a roam — so we re-read
    // it every push and diff the result. The walk is cheap; the diff-gate exists to skip the
    // pfctl -T load (and the table churn it causes) when the resolver set is unchanged, e.g. on a
    // static-ports or allowed-IP update that didn't touch DNS.
    const std::string disallowedDns = disallowedDnsTableDef();
    bool disallowedDnsReloaded = false;
    if (force || disallowedDns != lastDisallowedDnsDef_) {
        if (loadTableDef(disallowedDns)) {
            disallowedDnsReloaded = true;
        } else {
            ok = false;
        }
    }
    // Commit the text-diff baselines only if the whole apply succeeded; lastConfig_ is left untouched
    // by enable() on failure, so leaving these stale too keeps the next push's diff-gate consistent
    // (it re-evaluates and re-applies rather than skipping a piece that never actually landed).
    if (ok) {
        if (vpnReloaded) {
            lastVpnTrafficRules_ = vpnRules;
        }
        if (disallowedDnsReloaded) {
            lastDisallowedDnsDef_ = disallowedDns;
        }
    }
    return ok;
}

bool FirewallController::fullApply(const FirewallConfig &config)
{
    // Read the OS-resolver set once and cache it so applyTargeted's diff-gate has a baseline (and
    // doesn't redundantly reload <disallowed_dns> on the first steady-state push after this apply).
    lastDisallowedDnsDef_ = disallowedDnsTableDef();
    // Cache the vpn_traffic rules built from the current live interface state so applyTargeted's
    // diff-gate has a baseline; pass them into buildPfConf so the written ruleset and the cache come
    // from a single interface probe and can't diverge.
    lastVpnTrafficRules_ = vpnTrafficRules(config);
    const std::string pf = buildPfConf(config, lastDisallowedDnsDef_, lastVpnTrafficRules_);

    if (!IO::writeFile(WS_POSIX_CONFIG_DIR "/pf.conf", pf, 0600)) {
        spdlog::error("Could not write firewall rules: {}", IO::strerror(errno));
        return false;
    }

    // `pfctl -F all -f` flushes the active ruleset before loading the new one, so a parse failure
    // would leave pf empty (fail-open). Dry-run parse first (`-n` loads nothing); only flush and
    // load once we know the ruleset is valid, otherwise leave the existing rules in place.
    if (Utils::executeCommand("/sbin/pfctl", {"-n", "-f", WS_POSIX_CONFIG_DIR "/pf.conf"}) != 0) {
        spdlog::error("Generated pf ruleset failed to parse; leaving existing firewall rules intact");
        return false;
    }

    // Only mark enabled once the real load succeeds. Don't set enabled_ before the load: a failure
    // here would otherwise leave us claiming an applied ruleset that isn't.
    if (Utils::executeCommand("/sbin/pfctl", {"-v", "-F", "all", "-f", WS_POSIX_CONFIG_DIR "/pf.conf"}) != 0) {
        spdlog::error("pfctl failed to load the firewall ruleset");
        return false;
    }
    enabled_ = true;
    Utils::executeCommand("/sbin/pfctl", {"-e"});
    // A full reload flushes the <windscribe_dns> table; repopulate it from the last VPN DNS list.
    updateDns();
    return true;
}

bool FirewallController::loadTableDef(const std::string &tableDef)
{
    // Use a dedicated file so a targeted load never clobbers the main pf.conf. `pfctl -T load` only
    // updates table contents; it does not touch rules or the state table.
    if (!IO::writeFile(WS_POSIX_CONFIG_DIR "/pf_partial.conf", tableDef + "\n", 0600)) {
        spdlog::error("loadTableDef: could not write pf_partial.conf: {}", IO::strerror(errno));
        return false;
    }
    if (Utils::executeCommand("/sbin/pfctl", {"-T", "load", "-f", WS_POSIX_CONFIG_DIR "/pf_partial.conf"}) != 0) {
        spdlog::error("loadTableDef: pfctl failed to load table");
        return false;
    }
    return true;
}

bool FirewallController::loadAnchor(const std::string &anchorName, const std::vector<std::string> &rules)
{
    // `pfctl -a <anchor> -f` replaces only that anchor's sub-ruleset; the global ruleset and the pf
    // state table are left intact.
    std::string text;
    for (const auto &r : rules) {
        text += r + "\n";
    }

    if (!IO::writeFile(WS_POSIX_CONFIG_DIR "/pf_partial.conf", text, 0600)) {
        spdlog::error("loadAnchor: could not write pf_partial.conf: {}", IO::strerror(errno));
        return false;
    }
    if (Utils::executeCommand("/sbin/pfctl", {"-a", anchorName.c_str(), "-f", WS_POSIX_CONFIG_DIR "/pf_partial.conf"}) != 0) {
        spdlog::error("loadAnchor: pfctl failed to load anchor {}", anchorName);
        return false;
    }
    return true;
}

bool FirewallController::ourRulesPresent()
{
    std::string out;
    getRules(kFirewallAllRules, &out);
    // Require the default-deny backbone (block all) and the DNS-leak block in addition to our three
    // anchors. A ruleset carrying only the anchors but missing the backbone (e.g. partially
    // overwritten out-of-band) must not be adopted via the non-flushing targeted loads, which only
    // refresh tables/anchors and would never reinstall the missing scaffolding; returning false here
    // makes enable() fall back to a full reload that rebuilds the whole ruleset.
    return out.find("anchor \"" WS_PRODUCT_NAME_LOWER "_vpn_traffic\" all") != std::string::npos &&
           out.find("anchor \"" WS_PRODUCT_NAME_LOWER "_lan_traffic\" all") != std::string::npos &&
           out.find("anchor \"" WS_PRODUCT_NAME_LOWER "_static_ports_traffic\" all") != std::string::npos &&
           out.find("block drop all") != std::string::npos &&
           out.find("<disallowed_dns>") != std::string::npos;
}

void FirewallController::getRules(CmdFirewallRulesQuery query, std::string *outRules)
{
    // Fixed queries only — the table/anchor names are compile-time constants, never client input,
    // so a caller can't steer pfctl into showing an arbitrary table or anchor.
    if (query == kFirewallIpsTable) {
        Utils::executeCommand("/sbin/pfctl", {"-t", WS_PRODUCT_NAME_LOWER "_ips", "-T", "show"}, outRules, false);
    } else {
        Utils::executeCommand("/sbin/pfctl", {"-s", "rules"}, outRules, false);
    }
}

bool FirewallController::disable(bool keepPfEnabled)
{
    bool ok = Utils::executeCommand("/sbin/pfctl", {"-v", "-F", "all", "-f", "/etc/pf.conf"}) == 0;
    if (!keepPfEnabled) {
        ok = (Utils::executeCommand("/sbin/pfctl", {"-d"}) == 0) && ok;
    }
    enabled_ = false;
    // Drop the cached intent so the next enable() is treated as a fresh off->on transition (full
    // load) rather than an incremental update on top of a ruleset we just tore down.
    hasLastConfig_ = false;
    lastConfig_ = FirewallConfig();
    lastDisallowedDnsDef_.clear();
    lastVpnTrafficRules_.clear();
    return ok;
}

bool FirewallController::enabled()
{
    std::string output;

    Utils::executeCommand("/sbin/pfctl", {"-si"}, &output);
    return (output.find("Status: Enabled") != std::string::npos);
}

void FirewallController::setVpnDns(const std::vector<types::IpAddress> &dnsList)
{
    // Drop invalid entries (deserialised IpAddress with valid_ == false) so the pf table
    // never ends up with stray spaces or empty tokens. Stay in types::IpAddress form here —
    // string conversion happens once at the pf-config boundary in updateDns(), matching the
    // helper-internal convention from routes_manager / bound_route.
    vpnDns_.clear();
    vpnDns_.reserve(dnsList.size());
    for (const auto &dns : dnsList) {
        if (dns.isValid()) {
            vpnDns_.push_back(dns);
        }
    }

    if (vpnDns_.empty()) {
        spdlog::info("Setting VPN dns: empty");
    } else {
        std::string joined;
        for (const auto &dns : vpnDns_) {
            if (!joined.empty()) joined += ", ";
            joined += dns.toString();
        }
        spdlog::info("Setting VPN dns: {}", joined);
    }

    updateDns();
}

void FirewallController::updateDns()
{
    if (enabled_ && !vpnDns_.empty()) {
        loadDnsTable();
    } else {
        Utils::executeCommand("/sbin/pfctl", {"-T", WS_PRODUCT_NAME_LOWER "_dns", "flush"});
    }
}

void FirewallController::loadDnsTable()
{
    // pf tables accept space-separated dual-stack entries inside `{ ... }`. The pass/block rules
    // referring to <windscribe_dns> in buildPfConf are family-agnostic, so a mixed table covers
    // both v4 and v6 DNS lookups.
    std::string entries;
    for (const auto &dns : vpnDns_) {
        if (!entries.empty()) entries += " ";
        entries += dns.toString();
    }
    loadTableDef("table <" WS_PRODUCT_NAME_LOWER "_dns> persist { " + entries + " }");
}

std::string FirewallController::ipsTableDef(const std::vector<std::string> &ips)
{
    std::string s = "table <" WS_PRODUCT_NAME_LOWER "_ips> persist {";
    for (const auto &ip : ips) {
        s += ip + " ";
    }
    s += "}";
    return s;
}

std::string FirewallController::disallowedDnsTableDef()
{
    // <disallowed_dns> blocks the OS-configured system resolvers so they can't leak around the
    // VPN-pushed DNS. macOS stores both families in a flat list, and the block rules in buildPfConf
    // have no inet/inet6 selector, so a mixed v4 + v6 table covers both families.
    //
    // getOsDnsServers() reads SystemConfiguration ServerAddresses, which an unprivileged user can
    // populate (scutil/networksetup) or which may hold a non-literal value (e.g. a scoped
    // "fe80::1%en0"). A single unparseable token would fail the whole `pfctl -T load`, so validate
    // each entry as a bare IP literal and skip anything else — the same trust boundary the rest of
    // this controller applies to client-supplied tokens.
    std::string s = "table <disallowed_dns> persist {";
    for (const auto &ip : MacUtils::getOsDnsServers()) {
        if (!Validation::isValidIpAddress(ip)) {
            spdlog::error("disallowedDnsTableDef: skipping invalid OS DNS server \"{}\"", ip);
            continue;
        }
        s += ip + " ";
    }
    s += "}";
    return s;
}

std::vector<std::string> FirewallController::staticPortsRules(const std::vector<unsigned int> &ports)
{
    std::vector<std::string> rules;
    for (unsigned int port : ports) {
        rules.push_back("pass in quick proto tcp from any to any port = " + std::to_string(port));
    }
    return rules;
}

std::string FirewallController::buildPfConf(const FirewallConfig &config, const std::string &disallowedDnsDef, const std::vector<std::string> &vpnTrafficRulesText)
{
    std::string pf;
    pf += "# Automatically generated by " WS_PRODUCT_NAME ". Any manual change will be overridden.\n";

    // general options
    pf += "set block-policy drop\n";
    pf += "set fingerprints '/etc/pf.os'\n";
    pf += "set skip on lo0\n";

    // Apple anchor
    pf += "scrub-anchor \"com.apple/*\"\n";
    pf += "nat-anchor \"com.apple/*\"\n";
    pf += "rdr-anchor \"com.apple/*\"\n";
    pf += "dummynet-anchor \"com.apple/*\"\n";
    pf += "anchor \"com.apple/*\"\n";
    pf += "load anchor \"com.apple\" from \"/etc/pf.anchors/com.apple\"\n";

    // skip awdl and p2p interfaces (awdl Apple Wireless Direct Link and p2p related to AWDL features)
    for (const auto &iface : getAwdlP2pInterfaces()) {
        pf += "pass quick on " + iface + "\n";
    }

    // block malformed packets
    pf += "block in quick from no-route to any\n";
    pf += "block in quick from urpf-failed\n";

    // block inbound icmp echo requests
    pf += "pass proto icmp\n";
    pf += "block in quick inet proto icmp all icmp-type echoreq\n";

    // always allow DHCP
    pf += "pass out quick proto {tcp, udp} from any port {68} to any port {67}\n";
    pf += "pass in quick proto {tcp, udp} from any port {67} to any port {68}\n";
    pf += "pass out quick inet6 proto udp from any to any port {546}\n";
    pf += "pass in quick inet6 proto udp from any to any port {547}\n";

    // always allow igmp
    pf += "pass quick proto igmp allow-opts\n";

    // always allow esp/gre
    pf += "pass quick proto {esp, gre} from any to any\n";

    // block everything
    pf += "block all\n";

    // add app rules
    pf += ipsTableDef(config.allowedIps) + "\n";

    pf += "pass out quick inet from any to <" WS_PRODUCT_NAME_LOWER "_ips> no state\n";
    pf += "pass in quick inet from <" WS_PRODUCT_NAME_LOWER "_ips> to any no state\n";

    // this table is filled in by the helper (loadDnsTable)
    pf += "table <" WS_PRODUCT_NAME_LOWER "_dns> persist\n";
    // Allow VPN DNS, disallow other DNS
    pf += "pass out quick proto udp from any to <" WS_PRODUCT_NAME_LOWER "_dns> port 53\n";
    pf += "pass in quick proto udp from <" WS_PRODUCT_NAME_LOWER "_dns> port 53 to any\n";

    pf += disallowedDnsDef + "\n";
    pf += "block out quick proto {tcp, udp} from any to <disallowed_dns> port 53\n";
    pf += "block in quick proto {tcp, udp} from <disallowed_dns> port 53 to any\n";

    // LAN traffic rules have precedence over split tunnel rules. On an inclusive tunnel, whether
    // included apps can reach the LAN is controlled by the "Allow LAN traffic" setting.
    pf += anchorBlock(WS_PRODUCT_NAME_LOWER "_lan_traffic", lanTrafficRules(config.allowLanTraffic, config.isCustomConfig)) + "\n";
    pf += anchorBlock(WS_PRODUCT_NAME_LOWER "_vpn_traffic", vpnTrafficRulesText) + "\n";
    pf += anchorBlock(WS_PRODUCT_NAME_LOWER "_static_ports_traffic", staticPortsRules(config.staticPorts)) + "\n";

    return pf;
}

std::vector<std::string> FirewallController::lanTrafficRules(bool allowLanTraffic, bool isCustomConfig)
{
    std::vector<std::string> rules;

    // Always allow localhost (IPv4 and IPv6)
    rules.push_back("pass out quick inet from any to 127.0.0.0/8");
    rules.push_back("pass in quick inet from 127.0.0.0/8 to any");
    rules.push_back("pass out quick inet6 from any to ::1");
    rules.push_back("pass in quick inet6 from ::1 to any");

    // Always allow IPv6 link-local (required for Apple Continuity, neighbor discovery, etc.)
    rules.push_back("pass out quick inet6 from any to fe80::/10");
    rules.push_back("pass in quick inet6 from fe80::/10 to any");

    // Custom configs need private ranges allowed so third-party VPN DNS/gateway/routes work
    const std::string action = (allowLanTraffic || isCustomConfig) ? "pass" : "block";

    // Allow or block local network traffic based on setting.
    rules.push_back(action + " out quick inet from any to 192.168.0.0/16");
    rules.push_back(action + " in quick inet from 192.168.0.0/16 to any");
    rules.push_back(action + " out quick inet from any to 172.16.0.0/12");
    rules.push_back(action + " in quick inet from 172.16.0.0/12 to any");
    rules.push_back(action + " out quick inet from any to 169.254.0.0/16");
    rules.push_back(action + " in quick inet from 169.254.0.0/16 to any");
    rules.push_back("pass out quick inet from any to 10.255.255.0/24");
    rules.push_back("pass in quick inet from 10.255.255.0/24 to any");
    rules.push_back(action + " out quick inet from any to 10.0.0.0/8");
    rules.push_back(action + " in quick inet from 10.0.0.0/8 to any");

    // Multicast addresses (IPv4 and IPv6)
    rules.push_back(action + " out quick inet from any to 224.0.0.0/4");
    rules.push_back(action + " in quick inet from 224.0.0.0/4 to any");
    rules.push_back(action + " out quick inet6 from any to ff00::/8");
    rules.push_back(action + " in quick inet6 from ff00::/8 to any");

    // UPnP
    rules.push_back(action + " out quick inet proto udp from any to any port = 1900");
    rules.push_back(action + " in quick proto udp from any to any port = 1900");
    rules.push_back(action + " out quick inet proto udp from any to any port = 1901");
    rules.push_back(action + " in quick proto udp from any to any port = 1901");

    rules.push_back(action + " out quick inet proto udp from any to any port = 5350");
    rules.push_back(action + " in quick proto udp from any to any port = 5350");
    rules.push_back(action + " out quick inet proto udp from any to any port = 5351");
    rules.push_back(action + " in quick proto udp from any to any port = 5351");
    rules.push_back(action + " out quick inet proto udp from any to any port = 5353");
    rules.push_back(action + " in quick proto udp from any to any port = 5353");

    return rules;
}

std::vector<std::string> FirewallController::vpnTrafficRules(const FirewallConfig &config)
{
    std::vector<std::string> rules;
    const std::string &iface = config.vpnInterfaceName;

    if (!iface.empty()) {
        // One getifaddrs walk feeds both the local-address allow rules and the v6 capability gate.
        std::vector<std::string> localV4Addrs;
        bool hasV6Addr = false;
        probeInterfaceAddresses(iface, localV4Addrs, hasV6Addr);

        if (!config.isCustomConfig) {
            // Allow local addresses
            for (const auto &addr : localV4Addrs) {
                rules.push_back("pass out quick on " + iface + " inet from any to " + addr + "/32");
                rules.push_back("pass in quick on " + iface + " inet from " + addr + "/32 to any");
            }
            // Disallow RFC1918/link local/loopback traffic to go over tunnel
            rules.push_back("block out quick on " + iface + " inet from any to 192.168.0.0/16");
            rules.push_back("block in quick on " + iface + " inet from 192.168.0.0/16 to any");
            rules.push_back("block out quick on " + iface + " inet from any to 172.16.0.0/12");
            rules.push_back("block in quick on " + iface + " inet from 172.16.0.0/12 to any");
            rules.push_back("block out quick on " + iface + " inet from any to 169.254.0.0/16");
            rules.push_back("block in quick on " + iface + " inet from 169.254.0.0/16 to any");
            // Allow reserved subnet
            rules.push_back("pass out quick on " + iface + " inet from any to 10.255.255.0/24");
            rules.push_back("pass in quick on " + iface + " inet from 10.255.255.0/24 to any");
            // Disallow RFC1918/link local/loopback traffic to go over tunnel (cont'd)
            rules.push_back("block out quick on " + iface + " inet from any to 10.0.0.0/8");
            rules.push_back("block in quick on " + iface + " inet from 10.0.0.0/8 to any");
            rules.push_back("block out quick on " + iface + " inet from any to 127.0.0.0/8");
            rules.push_back("block in quick on " + iface + " inet from 127.0.0.0/8 to any");
            rules.push_back("block out quick on " + iface + " inet from any to 224.0.0.0/4");
            rules.push_back("block in quick on " + iface + " inet from 224.0.0.0/4 to any");
            // Disallow IPv6 local/link-local/multicast traffic to go over tunnel
            rules.push_back("block out quick on " + iface + " inet6 from any to ::1");
            rules.push_back("block in quick on " + iface + " inet6 from ::1 to any");
            rules.push_back("block out quick on " + iface + " inet6 from any to fe80::/10");
            rules.push_back("block in quick on " + iface + " inet6 from fe80::/10 to any");
            rules.push_back("block out quick on " + iface + " inet6 from any to ff00::/8");
            rules.push_back("block in quick on " + iface + " inet6 from ff00::/8 to any");
        }

        // Allow other traffic on VPN interface
        rules.push_back("pass out quick on " + iface + " inet from any to any");
        rules.push_back("pass in quick on " + iface + " inet from any to any");

        // Conditional v6 permit on the VPN adapter. Only enabled when a non-link-local v6 address
        // is actually present on the tunnel — gated to avoid leaks for v4-only tunnels.
        if (hasV6Addr) {
            spdlog::info("IPv6 permitted on VPN adapter: {}", iface);
            rules.push_back("pass out quick on " + iface + " inet6 from any to any");
            rules.push_back("pass in quick on " + iface + " inet6 from any to any");
        }

        // allow app group to send to any address, for split tunnel extension
        rules.push_back("pass out quick inet from any to any group " WS_PRODUCT_NAME_LOWER " flags any");
    }

    if (!config.connectingIp.empty()) {
        const std::string &ip = config.connectingIp;
        // only allow gid 0 and app group to send plaintext to VPN server
        // NB: set 'flags any' here, otherwise TCP-based VPN connections will drop if the firewall
        // is enabled after the connection
        rules.push_back("pass out quick inet from any to " + ip + " group 0 flags any");
        rules.push_back("pass out quick inet from any to " + ip + " group " WS_PRODUCT_NAME_LOWER " flags any");
        rules.push_back("pass in quick inet from " + ip + " to any flags any");
    }
    return rules;
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
        // Exact name match: a prefix match would pull "utun40"'s addresses into a lookup for
        // "utun4", emitting another interface's local address into this adapter's ACCEPT rules.
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

std::vector<std::string> FirewallController::getAwdlP2pInterfaces()
{
    std::vector<std::string> ret;
    std::string output;
    Utils::executeCommand("ifconfig", {"-l", "-u"}, &output);

    std::istringstream stream(output);
    std::string name;
    while (stream >> name) {
        if (name.rfind("p2p", 0) == 0 || name.rfind("awdl", 0) == 0 || name.rfind("llw", 0) == 0) {
            // Validate before interpolating into pf rule text ("pass quick on <iface>"), same as the
            // client-supplied vpnInterfaceName and the Linux getHotspotAdapter path: a name carrying
            // pf grammar characters must not reach the rule body.
            if (!Validation::isValidInterfaceName(name)) {
                spdlog::error("getAwdlP2pInterfaces: invalid interface name \"{}\", ignoring", name);
                continue;
            }
            ret.push_back(name);
        }
    }
    return ret;
}
