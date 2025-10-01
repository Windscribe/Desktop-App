#pragma once

#include <string>
#include <vector>

class FirewallController
{
public:
    inline static const std::string kTag = "Windscribe client rule";

    static FirewallController & instance()
    {
        static FirewallController fc;
        return fc;
    }

    bool enable(bool ipv6, const std::string &rules);
    void disable();
    bool enabled(const std::string &tag = kTag);
    void getRules(bool ipv6, std::string *outRules);

    void setSplitTunnelingEnabled(
        bool isConnected,
        bool isEnabled,
        bool isExclude,
        const std::string &adapter,
        const std::string &adapterIp);
    void setSplitTunnelIpExceptions(const std::vector<std::string> &ips);

private:
    FirewallController();
    ~FirewallController();

    bool connected_;
    bool splitTunnelEnabled_;
    bool splitTunnelExclude_;
    std::vector<std::string> splitTunnelIps_;
    std::string defaultAdapter_;
    std::string defaultAdapterIp_;
    std::string prevAdapter_;
    std::string netclassid_;

    void removeRuleInPosition(const std::string &chain, const std::string &rule, int position);
    void removeExclusiveIpRules();
    void removeInclusiveIpRules();
    void removeExclusiveAppRules();
    void removeInclusiveAppRules();
    void setSplitTunnelAppExceptions();
    void setSplitTunnelIngressRules(const std::string &defaultAdapterIp);
    void addRule(const std::vector<std::string> &args, bool prepend = false);
};
