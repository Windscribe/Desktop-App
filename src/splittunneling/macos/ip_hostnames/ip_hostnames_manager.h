#pragma once

#include <string>
#include <vector>

#include "../../../client/client-common/types/ipaddress.h"
#include "dns_resolver.h"

class IpHostnamesManager
{
public:
    explicit IpHostnamesManager();
    ~IpHostnamesManager();

    void enable();
    void disable();

    void setSettings(const std::vector<types::IpAddressRange> &ips, const std::vector<std::string> &hosts);

    bool isIpInList(const std::string &ip);

private:
    DnsResolver dnsResolver_;

    bool isEnabled_;
    std::mutex mutex_;

    // latest ips and hosts list
    std::vector<types::IpAddressRange> ipsLatest_;
    std::vector<std::string> hostsLatest_;

    std::vector<types::IpAddressRange> ipRanges_;
    std::set<std::string> addressesList_;

    void dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos);
};