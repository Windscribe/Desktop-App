#include "ip_hostnames_manager.h"
#include <spdlog/spdlog.h>

IpHostnamesManager::IpHostnamesManager(): isEnabled_(false)
{
    dnsResolver_.setResolveDomainsCallbackHandler(std::bind(&IpHostnamesManager::dnsResolverCallback, this, std::placeholders::_1));
}

IpHostnamesManager::~IpHostnamesManager()
{
    dnsResolver_.stop();
}

void IpHostnamesManager::enable()
{
    spdlog::debug("IpHostnamesManager::enable(), begin");
    {
        std::lock_guard<std::mutex> guard(mutex_);
        addressesList_.clear();
        addressesList_.insert(ipsLatest_.begin(), ipsLatest_.end());
        isEnabled_ = true;
    }

    dnsResolver_.cancelAll();
    dnsResolver_.resolveDomains(hostsLatest_);

    spdlog::debug("IpHostnamesManager::enable(), end");
}

void IpHostnamesManager::disable()
{
    spdlog::debug("IpHostnamesManager::disable(), begin");

    {
        std::lock_guard<std::mutex> guard(mutex_);

        if (!isEnabled_) {
            return;
        }
        isEnabled_ = false;
    }
    dnsResolver_.cancelAll();
    spdlog::debug("IpHostnamesManager::disable(), end");
}

void IpHostnamesManager::setSettings(const std::vector<std::string> &ips, const std::vector<std::string> &hosts)
{
    std::lock_guard<std::mutex> guard(mutex_);
    ipsLatest_ = ips;
    hostsLatest_ = hosts;
    spdlog::debug("IpHostnamesManager::setSettings(), end");
}

void IpHostnamesManager::dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos)
{
    std::lock_guard<std::mutex> guard(mutex_);

    for (auto it = hostInfos.begin(); it != hostInfos.end(); ++it) {
        if (!it->second.error) {
            for (const auto addr : it->second.addresses) {
                if (addr == "0.0.0.0") {
                    // ROBERT sometimes will give us an address of 0.0.0.0 for a 'blocked' resource.  This is not a valid address.
                    spdlog::debug("IpHostnamesManager::dnsResolverCallback(), Resolved : {}, IP: 0.0.0.0 (Ignored)", it->first);
                } else {
                    spdlog::debug("IpHostnamesManager::dnsResolverCallback(), Resolved : {}, IP: {}", it->first, addr);
                    addressesList_.insert(addr);
                }
            }
        } else {
            spdlog::warn("IpHostnamesManager::dnsResolverCallback(), Failed resolve : {}", it->first);
        }
    }
}

bool IpHostnamesManager::isIpInList(const std::string &ip)
{
    std::lock_guard<std::mutex> guard(mutex_);
    return addressesList_.count(ip) == 1;
}
