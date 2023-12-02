#pragma once

#include <string>
#include <vector>

class FirewallController
{
public:
    static FirewallController & instance()
    {
        static FirewallController fc;
        return fc;
    }

    bool enable(bool ipv6, const std::string &rules);
    void disable();
    bool enabled(const std::string &tag);
    void getRules(bool ipv6, std::string *outRules);

    void setSplitTunnelingEnabled(bool isConnected, bool isEnabled, bool isExclude);
    void setSplitTunnelExceptions(const std::vector<std::string> &ips);

private:
    FirewallController() : enabled_(false), connected_(false), splitTunnelEnabled_(false), splitTunnelExclude_(true) {};
    ~FirewallController() { disable(); };

    bool enabled_;
    bool connected_;
    bool splitTunnelEnabled_;
    bool splitTunnelExclude_;
    std::vector<std::string> splitTunnelIps_;

    void removeExclusiveRules();
    void removeInclusiveRules();
};
