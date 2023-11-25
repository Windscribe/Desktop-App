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

    bool enable(const std::string &rules, const std::string &table, const std::string &group);
    bool enabled();
    void disable(bool keepPfEnabled = false);
    void setSplitTunnelingEnabled(bool isEnabled, bool isExclude);
    void setSplitTunnelExceptions(const std::vector<std::string> &ips);
    void getRules(const std::string &table, const std::string &group, std::string *outRules);

private:
    FirewallController() : enabled_(false) {};
    ~FirewallController() { disable(); };

    bool enabled_;
    bool splitTunnelEnabled_;
    bool splitTunnelExclude_;
    std::vector<std::string> splitTunnelIps_;
};

