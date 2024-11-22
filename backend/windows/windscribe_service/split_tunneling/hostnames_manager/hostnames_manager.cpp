#include "../../all_headers.h"
#include <spdlog/spdlog.h>
#include "hostnames_manager.h"

HostnamesManager::HostnamesManager(): isEnabled_(false), isExcludeMode_(true),
    dnsResolver_(std::bind(&HostnamesManager::dnsResolverCallback, this, std::placeholders::_1))
{
}

HostnamesManager::~HostnamesManager()
{
}

void HostnamesManager::enable(const std::string &gatewayIp, unsigned long ifIndex)
{
    spdlog::debug("HostnamesManager::enable(), begin");
    {
        std::lock_guard<std::recursive_mutex> guard(mutex_);

        gatewayIp_ = gatewayIp;
        ifIndex_ = ifIndex;
        ipRoutes_.clear();
        ipRoutes_.setIps(gatewayIp_, ifIndex_, ipsLatestV4_);
        FirewallFilter::instance().setSplitTunnelingWhitelistIpsV4(ipsLatestV4_);
        FirewallFilter::instance().setSplitTunnelingWhitelistIpsV6(ipsLatestV6_);
        isEnabled_ = true;
    }

    dnsResolver_.cancelAll();
    dnsResolver_.resolveDomains(hostsLatest_);
    spdlog::debug("HostnamesManager::enable(), end");
}

void HostnamesManager::disable()
{
    spdlog::debug("HostnamesManager::disable(), begin");
    {
        std::lock_guard<std::recursive_mutex> guard(mutex_);

        if (!isEnabled_) {
            return;
        }
        ipRoutes_.clear();
        isEnabled_ = false;
    }
    dnsResolver_.cancelAll();
    spdlog::debug("HostnamesManager::disable(), end");
}

void HostnamesManager::setSettings(
    bool isExclude,
    const std::vector<Ip4AddressAndMask> &ipsV4,
    const std::vector<Ip6AddressAndPrefix> &ipsV6,
    const std::vector<std::string> &hosts)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    ipsLatestV4_ = ipsV4;
    ipsLatestV6_ = ipsV6;
    hostsLatest_ = hosts;
    isExcludeMode_ = isExclude;
    spdlog::debug("HostnamesManager::setSettings(), end");
}

void HostnamesManager::dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    std::vector<Ip4AddressAndMask> hostsIpV4;
    std::vector<Ip6AddressAndPrefix> hostsIpV6;

    for (auto it = hostInfos.begin(); it != hostInfos.end(); ++it) {
        if (!it->second.error) {
            std::vector<std::string> addresses = it->second.addresses;
            for (const auto addr : it->second.addresses) {
                if (addr == "0.0.0.0") {
                    // ROBERT sometimes will give us an address of 0.0.0.0 for a 'blocked' resource.  This is not a valid address.
                    spdlog::debug("IpHostnamesManager::dnsResolverCallback(), Resolved : {}, IP: 0.0.0.0 (Ignored)", it->first.c_str());
                } else {
                    spdlog::debug("RoutesManager::dnsResolverCallback(), Resolved : {}, IP: {}", it->first.c_str(), addr.c_str());
                    Ip4AddressAndMask ip4(addr.c_str());
                    if (ip4.isValid()) {
                        hostsIpV4.push_back(ip4);
                    } else {
                        Ip6AddressAndPrefix ip6(addr.c_str());
                        if (ip6.isValid()) {
                            hostsIpV6.push_back(ip6);
                        }
                    }
                }
            }
        } else {
            spdlog::warn("RoutesManager::dnsResolverCallback(), Failed resolve : {}", it->first.c_str());
        }
    }

    hostsIpV4.insert(hostsIpV4.end(), ipsLatestV4_.begin(), ipsLatestV4_.end());
    hostsIpV6.insert(hostsIpV6.end(), ipsLatestV6_.begin(), ipsLatestV6_.end());

    if (isEnabled_) {
        ipRoutes_.setIps(gatewayIp_, ifIndex_, hostsIpV4);
        FirewallFilter::instance().setSplitTunnelingWhitelistIpsV4(hostsIpV4);
        FirewallFilter::instance().setSplitTunnelingWhitelistIpsV6(hostsIpV6);
    }
}
