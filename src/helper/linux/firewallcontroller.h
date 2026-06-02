#pragma once

#include <string>
#include <vector>

#include "../common/helper_commands.h"
#include "types/ipaddress.h"

class FirewallController
{
public:
    inline static const std::string kTag = WS_PRODUCT_NAME " client rule";

    static FirewallController & instance()
    {
        static FirewallController fc;
        return fc;
    }

    // Builds the entire v4 and v6 iptables ruleset from the validated intent and applies it.
    // The helper is the sole author of the rules — no rule text crosses the IPC boundary.
    bool enable(const FirewallConfig &config);
    // Returns whether the kill switch is actually down afterward (our INPUT jumps gone). The
    // -D/-F/-X teardown ignores per-command exit codes (they return non-zero for already-absent
    // rules), so success is reported by verifying the post-state instead.
    bool disable();
    bool enabled(const std::string &tag = kTag);

    // adapterIp / adapterIpV6 — the physical default-adapter addresses, used by the
    // ingress-connmark path (`setSplitTunnelIngressRules`). Either may be empty
    // (no v6 connectivity, or v4 not yet detected); the empty family is silently
    // skipped instead of generating bogus shell commands.
    void setSplitTunnelingEnabled(
        bool isConnected,
        bool isEnabled,
        bool isExclude,
        const std::string &adapter,
        const std::string &adapterIp,
        const std::string &adapterIpV6);

    // Dual-stack split-tunnel exception list. Each entry is dispatched to
    // `iptables` or `ip6tables` by family at the shell-command boundary.
    void setSplitTunnelIpExceptions(const std::vector<types::IpAddressRange> &ips);

private:
    FirewallController();
    ~FirewallController();

    bool connected_;
    bool splitTunnelEnabled_;
    bool splitTunnelExclude_;
    std::vector<types::IpAddressRange> splitTunnelIps_;
    std::string defaultAdapter_;
    std::string defaultAdapterIp_;
    std::string defaultAdapterIpV6_;
    std::string prevAdapter_;
    std::string netclassid_;

    void removeExclusiveIpRules();
    void removeInclusiveIpRules();
    void removeExclusiveAppRules();
    void removeInclusiveAppRules();
    void setSplitTunnelAppExceptions();
    void setSplitTunnelIngressRules(const std::string &defaultAdapterIp, const std::string &defaultAdapterIpV6);
    // isV6 picks `ip6tables` over `iptables` for the underlying shell command. The
    // selectors emitted (cgroup, mark, comment, …) are identical for both families.
    void addRule(const std::vector<std::string> &args, bool prepend = false, bool isV6 = false);

    // Rule construction (moved out of the engine so the client only sends intent). bExists is the
    // per-family probe of whether our INPUT jump is already installed, so the INPUT/OUTPUT jumps are
    // (re-)inserted only when missing. enable() probes each family separately so a family whose jump
    // was torn down out-of-band gets it re-inserted instead of left fail-open. localV4Addrs and
    // hasV6Addr come from a single getifaddrs walk in enable() (probeInterfaceAddresses) so the v4
    // and v6 builders don't each re-scan the interface table.
    std::string buildRulesV4(const FirewallConfig &config, bool bExists, const std::vector<std::string> &localV4Addrs);
    std::string buildRulesV6(const FirewallConfig &config, bool bExists, bool hasV6Addr);
    // True when the kernel has an active IPv6 stack (checks /proc/net/if_inet6). When false,
    // ip6tables can't operate and there's no v6 traffic to filter, so enable() skips the v6 ruleset
    // instead of treating its failure as a kill-switch failure.
    bool ipv6Available();
    // Write a family's rule blob to its run-dir file (0600). Split from the restore so enable() can
    // write and validate both families before committing either.
    bool writeRulesFile(bool ipv6, const std::string &rules);
    // Run *tables-restore on the family's already-written file. testOnly adds --test, which parses
    // and constructs the ruleset without committing it (the pre-apply validation pass).
    bool restoreRules(bool ipv6, bool testOnly);
    // True if the INPUT -> windscribe_input jump exists in the given family.
    bool inputJumpExists(bool isV6, const std::string &tag = kTag);

    // Local-machine introspection, performed in the privileged helper rather than trusted from the
    // client. One getifaddrs walk fills both outputs: the interface's v4 addresses, and whether it
    // carries a non-link-local v6 address (link-local is auto-configured on every v6-capable iface,
    // so it isn't a signal of real v6 capability).
    void probeInterfaceAddresses(const std::string &iface, std::vector<std::string> &v4Addrs, bool &hasNonLinkLocalV6);
    std::string getHotspotAdapter();
    // v4 block-jump probe for the given built-in chain ("INPUT"/"OUTPUT"), to keep the restore blob
    // idempotent under `iptables-restore -n` (which does not flush existing chains). Probed per chain
    // so a jump torn down on only one chain is re-added without duplicating the other.
    bool hasBlockJump(const char *chain);

    // 1-based OUTPUT-chain position of the dns-leak-protect jump (`-j windscribe_dnsleaks`) for the
    // given family, or 0 if absent. tunnelAdapterSet drives the conservative fallback when the live
    // table can't be read. See the .cpp for the residual TOCTOU note.
    int dnsLeaksJumpPos(bool isIPv6, bool tunnelAdapterSet);
    // Builds the `-I OUTPUT [pos] -j windscribe_output` jump, positioned directly below the
    // dns-leak-protect jump when present so the dnsleaks DROPs keep precedence (see dnsLeaksJumpPos).
    // Shared by buildRulesV4/buildRulesV6 so the ordering fix can never apply to only one family.
    std::string outputJumpRule(bool isIPv6, bool tunnelAdapterSet, const std::string &commentSuffix);
};
