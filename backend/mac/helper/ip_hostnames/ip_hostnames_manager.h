#pragma once

#include "dns_resolver.h"
#include "ip_routes.h"

#include <string>
#include <vector>

class IpHostnamesManager
{
public:
    explicit IpHostnamesManager();
    ~IpHostnamesManager();

    void enable(const std::string &gatewayIp);
    void disable();
    void setSettings(const std::vector<std::string> &ips, const std::vector<std::string> &hosts);

private:
    DnsResolver dnsResolver_;
    IpRoutes ipRoutes_;

    bool isEnabled_;
    std::mutex mutex_;

    // latest ips and hosts list
    std::vector<std::string> ipsLatest_;
    std::vector<std::string> hostsLatest_;

    std::string gatewayIp_;

    void dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos);
};