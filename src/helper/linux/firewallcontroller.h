#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "../common/helper_commands.h"
#include "types/ipaddress.h"

// Linux firewall + split tunneling, implemented as a single nftables table `inet windscribe`
// managed via libnftables (NftablesController). The helper is the sole author of all rule text; only
// the validated FirewallConfig crosses the IPC boundary.
//
// Table layout (this class owns the firewall and split-tunnel chains; the dnsleaks chain is owned
// by the DNS-leak path, the wg_* chains by WireGuardAdapter, all in the same table):
//   chain input  { type filter hook input  priority -10; policy drop; ... ; jump st_in  }
//   chain output { type filter hook output priority -10; policy drop; ... ; jump st_out }
//   chain st_in / st_out               regular chains, split-tunnel exceptions (cgroup/per-IP ACCEPTs)
//   chain st_route_out                 type route hook output  (mangle): connmark restore + cgroup MARK
//   chain st_nat_post                  type nat   hook postrouting (srcnat): masquerade
//   chain st_pre_cm                    type filter hook prerouting (mangle): connmark set
//
// `inet` carries both v4 and v6 in one ruleset, so there is no per-family duplication and each apply
// is a single atomic nftables transaction. We own base chains (policy drop) rather than jumping into
// the built-in INPUT/OUTPUT chains.
class FirewallController
{
public:
    inline static const std::string kTag = WS_PRODUCT_NAME " client rule";

    static FirewallController & instance()
    {
        static FirewallController fc;
        return fc;
    }

    // Builds and applies the firewall input/output chains from the validated intent, then
    // re-applies the split-tunnel chains. Returns false if the generated ruleset fails the commit
    // (applied directly as one atomic transaction; a rejected ruleset fails as a unit).
    bool enable(const FirewallConfig &config);
    // Deletes the firewall + split-tunnel chains (leaving the dnsleaks / wg_* / boot chains and the
    // table itself intact). Returns whether the firewall is actually down afterward.
    bool disable();
    // True if the firewall output chain exists in the table.
    bool enabled();

    // adapterIp / adapterIpV6 — the physical default-adapter addresses for the ingress-connmark path.
    // Either may be empty; the empty family is skipped.
    void setSplitTunnelingEnabled(
        bool isConnected,
        bool isEnabled,
        bool isExclude,
        const std::string &adapter,
        const std::string &adapterIp,
        const std::string &adapterIpV6);

    // Stores the dual-stack split-tunnel exception list (each entry emitted with its family's nft
    // qualifier) and, by default (applyNow=true), applies it as one nft transaction — so a caller that
    // merely sets exceptions can never silently leave them uninstalled. Pass applyNow=false to store
    // WITHOUT applying, valid ONLY when a setSplitTunnelingEnabled() (which rebuilds and applies from
    // full state) is guaranteed to follow, e.g. the connect path, to avoid a wasted transaction.
    // Returns false if applyNow and the nft apply was rejected.
    bool setSplitTunnelIpExceptions(const std::vector<types::IpAddressRange> &ips, bool applyNow = true);
    // Rebuilds and applies the split-tunnel chains from the current state as one nft transaction.
    // Call after setSplitTunnelIpExceptions() outside the connect path. Returns false (and logs) if
    // the nft apply is rejected, so the failure isn't swallowed.
    bool applySplitTunnel();

private:
    FirewallController();
    ~FirewallController();

    // Serializes all access to the split-tunnel state below. enable() and the split-tunnel setters run
    // on the IPC thread, while the async hostname-resolution path (HostnamesManager::dnsResolverCallback)
    // drives applySplitTunnel() from the DNS-resolver thread. Without this lock the two threads can read
    // and write splitTunnelIps_ (a std::vector) concurrently — a data race / UB — and buildSplitTunnelRules
    // could emit a transaction from a half-updated snapshot. Recursive so a setter can hold the lock
    // across its mutate-then-apply (the apply re-enters via applySplitTunnel()) and stay atomic.
    std::recursive_mutex stateMutex_;
    bool connected_;
    bool splitTunnelEnabled_;
    bool splitTunnelExclude_;
    std::vector<types::IpAddressRange> splitTunnelIps_;
    std::string defaultAdapter_;
    std::string defaultAdapterIp_;
    std::string defaultAdapterIpV6_;

    // Builds the `add rule inet windscribe input/output ...` lines for the firewall (no chain
    // creation, no jump) from the validated intent. Pure: depends only on its arguments and
    // local-machine introspection passed in, so it is unit-testable as text. localV4Addrs and
    // hasV6Addr come from a single getifaddrs walk (probeInterfaceAddresses).
    // Sets ok=false (and logs) when a required rule cannot be built (e.g. getpwnam for the service user
    // fails); enable() then aborts the apply rather than committing a ruleset that blocks the VPN.
    std::string buildFirewallRules(const FirewallConfig &config,
                                     const std::vector<std::string> &localV4Addrs,
                                     bool hasV6Addr, bool &ok);
    // Emits the `add/delete/add chain` + rule lines for the split-tunnel chains
    // (st_in/st_out/st_route_out/st_nat_post/st_pre_cm) from the current split-tunnel state, WITHOUT
    // the enclosing `add table`. enable() folds this into its own transaction (so the input/output
    // jumps to st_in/st_out resolve); applySplitTunnel() wraps it in a standalone transaction.
    std::string buildSplitTunnelRules();

    // Local-machine introspection performed in the privileged helper (not trusted from the client):
    // one getifaddrs walk yields the interface's v4 addresses and whether it carries a non-link-local
    // v6 address.
    void probeInterfaceAddresses(const std::string &iface, std::vector<std::string> &v4Addrs, bool &hasNonLinkLocalV6);
    std::string getHotspotAdapter();
    // One-shot teardown of any pre-nftables iptables `windscribe_*` chains / dnsleaks chain left by an
    // older install, so they can't shadow or leak alongside the new table. Idempotent no-ops once the
    // legacy state is gone. Runs once per helper process (constructor).
    void purgeLegacyIptables();
};
