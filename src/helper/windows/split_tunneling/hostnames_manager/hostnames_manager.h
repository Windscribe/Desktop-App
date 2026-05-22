#pragma once

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "dns_resolver.h"
#include "ip_routes.h"

#include "types/ipaddress.h"

class CalloutFilter;

class HostnamesManager
{
public:
    // calloutFilter is optional: when set, whitelist IP updates are mirrored to it so
    // the v6 inclusive-mode anti-leak BLOCK can be bypassed for hostname-routed v6
    // destinations regardless of which app issues the connection. Owner outlives this.
    explicit HostnamesManager(CalloutFilter *calloutFilter = nullptr);
    ~HostnamesManager();

    // gatewayIp / gatewayIpV6 — default gateways for the active adapter (either may be invalid).
    void enable(const types::IpAddress &gatewayIp,
                const types::IpAddress &gatewayIpV6,
                unsigned long ifIndex);
    void disable();

    // Dual-stack: ips holds IPv4 and/or IPv6 ranges; family is dispatched by isV4()/isV6().
    void setSettings(bool isExclude,
                     const std::vector<types::IpAddressRange> &ips,
                     const std::vector<std::string> &hosts);

private:
    CalloutFilter *calloutFilter_;
    DnsResolver dnsResolver_;
    IpRoutes ipRoutes_;

    bool isEnabled_;
    std::recursive_mutex mutex_;

    bool isExcludeMode_;

    // Latest IPs (dual-stack) and hosts list configured via setSettings().
    std::vector<types::IpAddressRange> ipsLatest_;
    std::vector<std::string> hostsLatest_;

    types::IpAddress gatewayIp_;
    types::IpAddress gatewayIpV6_;
    unsigned long ifIndex_;

    void dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos);
};
