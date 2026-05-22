#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "dns_resolver.h"
#include "ip_routes.h"
#include "types/ipaddress.h"

class HostnamesManager
{
public:
    explicit HostnamesManager();
    ~HostnamesManager();

    // gatewayIp / gatewayIpV6 — gateways for the active "outside" adapter (either may be
    // invalid). For WireGuard inclusive mode the caller passes the WG adapter's own
    // address as the gateway, mirroring the existing v4 convention.
    void enable(const types::IpAddress &gatewayIp, const types::IpAddress &gatewayIpV6);
    void disable();

    // Dual-stack: ips holds IPv4 and/or IPv6 ranges; family is dispatched by isV4()/isV6().
    void setSettings(const std::vector<types::IpAddressRange> &ips,
                     const std::vector<std::string> &hosts);

private:
    DnsResolver dnsResolver_;
    IpRoutes ipRoutes_;

    bool isEnabled_;
    std::recursive_mutex mutex_;

    // Latest IPs (dual-stack) and hosts list configured via setSettings().
    std::vector<types::IpAddressRange> ipsLatest_;
    std::vector<std::string> hostsLatest_;

    types::IpAddress gatewayIp_;
    types::IpAddress gatewayIpV6_;

    void dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos);
};
