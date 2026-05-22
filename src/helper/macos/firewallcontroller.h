#pragma once

#include <string>
#include <vector>

#include "types/ipaddress.h"

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

    // Populates the <windscribe_dns> pf table with the VPN-pushed nameservers. The list may
    // contain a mix of IPv4 and IPv6 addresses; pf tables accept dual-stack entries natively
    // and the matching rules in firewallcontroller_mac.cpp use family-agnostic
    // `pass {out,in} quick proto udp from any to <windscribe_dns> port 53`. An empty / all-
    // invalid list flushes the table (no DNS allowed through the kill-switch).
    void setVpnDns(const std::vector<types::IpAddress> &dnsList);

private:
    FirewallController();
    ~FirewallController();
    void updateDns();

    bool enabled_;
    std::vector<types::IpAddress> vpnDns_;
};
