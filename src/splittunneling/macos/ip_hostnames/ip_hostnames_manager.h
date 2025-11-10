#pragma once

#include "dns_resolver.h"

#include <string>
#include <vector>

class IpHostnamesManager
{
public:
    explicit IpHostnamesManager();
    ~IpHostnamesManager();

    void enable();
    void disable();

    void setSettings(const std::vector<std::string> &ips, const std::vector<std::string> &hosts);

    bool isIpInList(const std::string &ip);

private:
    DnsResolver dnsResolver_;

    bool isEnabled_;
    std::mutex mutex_;

    // latest ips and hosts list
    std::vector<std::string> ipsLatest_;
    std::vector<std::string> hostsLatest_;

    std::set<std::string> addressesList_;

    void dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos);
};