#pragma once

#include <string>
#include <vector>

#include "../common/helper_commands.h"
#include "types/ipaddress.h"

class FirewallController
{
public:
    static FirewallController & instance()
    {
        static FirewallController fc;
        return fc;
    }

    // Builds the entire pf ruleset (options, anchors, tables) from the validated intent and
    // applies it. The helper is the sole author of the rules — no pf text crosses the IPC boundary.
    bool enable(const FirewallConfig &config);
    bool enabled();
    // Returns whether the pf teardown commands succeeded.
    bool disable(bool keepPfEnabled = false);
    void getRules(CmdFirewallRulesQuery query, std::string *outRules);

    // Populates the <windscribe_dns> pf table with the VPN-pushed nameservers. The list may
    // contain a mix of IPv4 and IPv6 addresses; pf tables accept dual-stack entries natively
    // and the matching rules in firewallcontroller_mac.cpp use family-agnostic
    // `pass {out,in} quick proto udp from any to <windscribe_dns> port 53`. An empty / all-
    // invalid list flushes the table (no DNS allowed through the firewall).
    void setVpnDns(const std::vector<types::IpAddress> &dnsList);

private:
    FirewallController();
    ~FirewallController();
    void updateDns();
    // Loads the <windscribe_dns> pf table from vpnDns_ via `pfctl -T load` (a table update only,
    // never arbitrary rules). Used by updateDns() so a DNS change doesn't require a full rebuild.
    void loadDnsTable();

    // Off->on transition: write and load the whole pf.conf via `pfctl -F all -f`. This is the only
    // path that flushes the pf state table — steady-state updates go through the incremental loads
    // below so established connections aren't disturbed. Returns false if the ruleset could not be
    // written or loaded, so the caller can avoid caching a config that was never applied.
    bool fullApply(const FirewallConfig &config);
    // Steady-state path: reload only the tables/anchors that changed via the state-preserving
    // targeted loads. When force is true (adopting a ruleset left by a previous helper instance,
    // with no cached config to diff against) every piece is reloaded. Returns false if any load
    // failed, so enable() can avoid caching an intent the active ruleset doesn't match.
    bool applyTargeted(const FirewallConfig &config, bool force);
    // Targeted updates that preserve pf state: `pfctl -T load` for a single table and
    // `pfctl -a <anchor> -f` for a single anchor's sub-ruleset. Return false if the file could not
    // be written or pfctl rejected the load, so enable() can avoid caching an intent that the
    // active ruleset doesn't match.
    bool loadTableDef(const std::string &tableDef);
    bool loadAnchor(const std::string &anchorName, const std::vector<std::string> &rules);
    // True iff our anchors are currently present in the active pf ruleset (used to adopt a ruleset
    // left behind by a previous helper instance without a state-flushing full reload).
    bool ourRulesPresent();

    // Rule construction, moved out of the engine so the client only sends intent. Shared by the
    // full build (buildPfConf) and the incremental anchor/table reloads. buildPfConf takes the
    // <disallowed_dns> table definition and the lan_traffic/vpn_traffic rules precomputed by the
    // caller so the OS-resolver set (getOsDnsServers) and the interface probe each run only once
    // per full apply. The caller runs probeInterfaceAddresses once and feeds both anchor builders:
    // lanTrafficRules needs the adapter's local v4 addresses for its tunnel carve-out and the v6
    // capability for its ICMPv6 error exemption, vpnTrafficRules needs the v6 capability for its
    // conditional inet6 pass.
    std::string buildPfConf(const FirewallConfig &config, const std::string &disallowedDnsDef, const std::vector<std::string> &lanTrafficRulesText, const std::vector<std::string> &vpnTrafficRulesText);
    std::string ipsTableDef(const std::vector<std::string> &ips);
    // The <disallowed_dns> table definition (the host's OS resolvers, each validated as an IP
    // literal before interpolation). Shared by the full build and the targeted refresh so a change
    // can never go to only one of them.
    std::string disallowedDnsTableDef();
    std::vector<std::string> vpnTrafficRules(const FirewallConfig &config, bool hasV6Addr);
    std::vector<std::string> lanTrafficRules(const FirewallConfig &config, const std::vector<std::string> &localV4Addrs, bool hasV6Addr);
    std::vector<std::string> staticPortsRules(const std::vector<unsigned int> &ports);

    // Local-machine introspection, performed in the privileged helper rather than trusted from the
    // client. One getifaddrs walk fills both outputs: the interface's v4 addresses, and whether it
    // carries a non-link-local v6 address (link-local is auto-configured on every v6-capable iface,
    // so it isn't a signal of real v6 capability).
    void probeInterfaceAddresses(const std::string &iface, std::vector<std::string> &v4Addrs, bool &hasNonLinkLocalV6);

    bool enabled_;
    std::vector<types::IpAddress> vpnDns_;

    // Last applied intent, so enable() can diff and reload only what changed (mirrors the engine's
    // old incremental anchor/table updates). hasLastConfig_ is cleared on disable() so the next
    // enable() is treated as a fresh off->on transition.
    FirewallConfig lastConfig_;
    bool hasLastConfig_ = false;
    // Last <disallowed_dns> table definition we loaded, so applyTargeted only re-walks/reloads the
    // OS-resolver set when it actually changed instead of on every push.
    std::string lastDisallowedDnsDef_;
    // Last lan_traffic/vpn_traffic anchor rules we loaded. Both depend on live interface state that
    // isn't in FirewallConfig (the LAN anchor's tunnel carve-out bakes in the VPN interface name and
    // its local v4 addresses; the VPN anchor bakes in the v6 capability), so applyTargeted diffs the
    // generated rules against these to reload an anchor whenever the tunnel's identity/addresses
    // change — e.g. a reconnect swapping utun4 -> utun5 — not only on a config-field change.
    std::vector<std::string> lastLanTrafficRules_;
    std::vector<std::string> lastVpnTrafficRules_;
};
