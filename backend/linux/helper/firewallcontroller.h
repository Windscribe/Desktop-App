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
        const std::string &adapter);
    void setSplitTunnelIpExceptions(const std::vector<std::string> &ips);

private:
    FirewallController() : connected_(false), splitTunnelEnabled_(false), splitTunnelExclude_(true) {};
    ~FirewallController() { disable(); };

    bool connected_;
    bool splitTunnelEnabled_;
    bool splitTunnelExclude_;
    std::vector<std::string> splitTunnelIps_;
    std::string defaultAdapter_;
    std::string prevAdapter_;
    std::string netclassid_;

    void removeExclusiveIpRules();
    void removeInclusiveIpRules();
    void removeExclusiveAppRules();
    void removeInclusiveAppRules();
    void setSplitTunnelAppExceptions();
    void addRule(const std::vector<std::string> &args);
};
