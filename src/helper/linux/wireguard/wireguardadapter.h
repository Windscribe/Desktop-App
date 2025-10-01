#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

class WireGuardAdapter final
{
public:
    explicit WireGuardAdapter(const std::string &name);
    ~WireGuardAdapter();

    bool setIpAddress(const std::string &address);
    bool setDnsServers(const std::string &addressList, const std::string &scriptName);
    bool enableRouting(const std::string &ipAddress, const std::vector<std::string> &allowedIps, uint32_t fwmark);
    bool disableRouting();

    const std::string getName() const { return name_; }
    bool hasDefaultRoute() const { return has_default_route_; }


private:
    bool flushDnsServer();

    std::string name_;
    std::string comment_;
    std::string dns_script_name_;
    bool is_dns_server_set_;
    std::string dns_script_command_;
    bool has_default_route_;

    std::vector<std::string> allowedIps_;
    uint32_t fwmark_;

    bool addFirewallRules(const std::string &ipAddress, uint32_t fwmark);
    bool removeFirewallRules();
};
