#pragma once

#include <string>
#include <vector>

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

    bool enable(bool ipv6, const std::string &rules);
    void disable();
    bool enabled(const std::string &tag = kTag);
    void getRules(bool ipv6, std::string *outRules);

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
};
