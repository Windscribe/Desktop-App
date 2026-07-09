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

    // address may be a single IPv4/IPv6 CIDR or a comma/semicolon/space-separated
    // dual-stack list (e.g. "10.245.6.78/32, fd00:abcd::1/128"). The IPv4 segment
    // is mandatory (WireGuard control traffic and IpValidation::validateWireGuardConfig
    // both require it); IPv6 is optional and skipped when not present.
    bool setIpAddress(const std::string &address);
    bool setDnsServers(const std::string &addressList, const std::string &scriptName);
    bool enableRouting(const std::vector<std::string> &allowedIps, uint32_t fwmark, uint32_t routingTable);
    bool disableRouting();

    const std::string getName() const { return name_; }
    bool hasDefaultRoute() const { return has_default_route_v4_ || has_default_route_v6_; }


private:
    bool flushDnsServer();

    std::string name_;
    std::string dns_script_name_;
    bool is_dns_server_set_;
    std::vector<std::string> dns_script_env_;

    // Per-family default-route state. Used by disableRouting to tear down only
    // what enableRouting actually installed.
    bool has_default_route_v4_;
    bool has_default_route_v6_;

    // Parsed client address segments populated by setIpAddress, reused in
    // enableRouting/addFirewallRules. ipv4Cidr_ is mandatory; ipv6Cidr_ is empty
    // when the WG server did not push a v6 client address.
    std::string ipv4Cidr_;
    std::string ipv6Cidr_;

    std::vector<std::string> allowedIps_;
    uint32_t routingTable_;

    bool addFirewallRules(uint32_t fwmark);
    bool removeFirewallRules();
};
