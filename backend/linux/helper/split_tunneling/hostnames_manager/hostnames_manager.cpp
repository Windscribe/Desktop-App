#include "hostnames_manager.h"

#include "../../firewallcontroller.h"
#include "../../logger.h"

HostnamesManager::HostnamesManager(): isEnabled_(false)
{
    dnsResolver_.setResolveDomainsCallbackHandler(std::bind(&HostnamesManager::dnsResolverCallback, this, std::placeholders::_1));
}

HostnamesManager::~HostnamesManager()
{
    dnsResolver_.stop();
}

void HostnamesManager::enable(const std::string &gatewayIp)
{
    Logger::instance().out("HostnamesManager::enable(), begin");
    {
        std::lock_guard<std::recursive_mutex> guard(mutex_);

        gatewayIp_ = gatewayIp;
        ipRoutes_.clear();
        ipRoutes_.setIps(gatewayIp_, ipsLatest_);
        FirewallController::instance().setSplitTunnelExceptions(ipsLatest_);
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

        if (!isEnabled_) {
            return;
        }
        ipRoutes_.clear();
        isEnabled_ = false;
    }
    dnsResolver_.cancelAll();
    Logger::instance().out("HostnamesManager::disable(), end");
}

void HostnamesManager::setSettings(const std::vector<std::string> &ips, const std::vector<std::string> &hosts)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    ipsLatest_ = ips;
    hostsLatest_ = hosts;
    Logger::instance().out("HostnamesManager::setSettings(), end");
}

void HostnamesManager::dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    std::vector<std::string> hostsIps;

    for (auto it = hostInfos.begin(); it != hostInfos.end(); ++it) {
        if (!it->second.error) {
            for (const auto addr : it->second.addresses) {
                Logger::instance().out("HostnamesManager::dnsResolverCallback(), Resolved : %s, IP: %s", it->first.c_str(), addr.c_str());
            }
            hostsIps.insert(hostsIps.end(), it->second.addresses.begin(), it->second.addresses.end());
        } else {
            Logger::instance().out("HostnamesManager::dnsResolverCallback(), Failed resolve : %s", it->first.c_str());
        }
    }

    hostsIps.insert(hostsIps.end(), ipsLatest_.begin(), ipsLatest_.end());

    if (isEnabled_) {
        ipRoutes_.setIps(gatewayIp_, hostsIps);
        FirewallController::instance().setSplitTunnelExceptions(hostsIps);
    }
}
