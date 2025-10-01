#pragma once

#include <string>

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
    void getRules(const std::string &table, const std::string &group, std::string *outRules);
    void setVpnDns(const std::string &dns);

private:
    FirewallController();
    ~FirewallController();
    void updateDns();

    bool enabled_;
    std::string windscribeDns_;
};
