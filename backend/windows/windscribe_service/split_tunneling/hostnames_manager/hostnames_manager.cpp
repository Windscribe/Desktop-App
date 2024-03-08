#include "../../all_headers.h"
#include "hostnames_manager.h"
#include "../../logger.h"

HostnamesManager::HostnamesManager(): isEnabled_(false), isExcludeMode_(true),
    dnsResolver_(std::bind(&HostnamesManager::dnsResolverCallback, this, std::placeholders::_1))
{
}

HostnamesManager::~HostnamesManager()
{
}

void HostnamesManager::enable(const std::string &gatewayIp, unsigned long ifIndex)
{
    Logger::instance().out("HostnamesManager::enable(), begin");
    {
        std::lock_guard<std::recursive_mutex> guard(mutex_);

        gatewayIp_ = gatewayIp;
        ifIndex_ = ifIndex;
        ipRoutes_.clear();
        ipRoutes_.setIps(gatewayIp_, ifIndex_, ipsLatest_);
        FirewallFilter::instance().setSplitTunnelingWhitelistIps(ipsLatest_);
        isEnabled_ = true;
    }

    dnsResolver_.cancelAll();
    dnsResolver_.resolveDomains(hostsLatest_);
    Logger::instance().out("HostnamesManager::enable(), end");
}

void HostnamesManager::disable()
{
    Logger::instance().out("HostnamesManager::disable(), begin");

    {
        std::lock_guard<std::recursive_mutex> guard(mutex_);

        if (!isEnabled_)
        {
            return;
        }
        ipRoutes_.clear();
        isEnabled_ = false;
    }
    dnsResolver_.cancelAll();
    Logger::instance().out("HostnamesManager::disable(), end");
}

void HostnamesManager::setSettings(bool isExclude, const std::vector<Ip4AddressAndMask> &ips, const std::vector<std::string> &hosts)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    ipsLatest_ = ips;
    hostsLatest_ = hosts;
    isExcludeMode_ = isExclude;
    Logger::instance().out("HostnamesManager::setSettings(), end");

}

void HostnamesManager::dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    std::vector<Ip4AddressAndMask> hostsIps;
    for (auto it = hostInfos.begin(); it != hostInfos.end(); ++it)
    {
        if (!it->second.error)
        {
            std::vector<std::string> addresses = it->second.addresses;
            for (auto addr = addresses.begin(); addr != addresses.end(); ++addr)
            {
                hostsIps.push_back(Ip4AddressAndMask(addr->c_str()));
                Logger::instance().out("RoutesManager::dnsResolverCallback(), Resolved : %s, IP: %s", it->first.c_str(), addr->c_str());
            }
        }
        else
        {
            Logger::instance().out("RoutesManager::dnsResolverCallback(), Failed resolve : %s", it->first.c_str());
        }
    }

    hostsIps.insert(hostsIps.end(), ipsLatest_.begin(), ipsLatest_.end());

    if (isEnabled_)
    {
        ipRoutes_.setIps(gatewayIp_, ifIndex_, hostsIps);
        FirewallFilter::instance().setSplitTunnelingWhitelistIps(hostsIps);
    }
}
