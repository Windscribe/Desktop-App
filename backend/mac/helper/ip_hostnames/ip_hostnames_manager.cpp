#include "ip_hostnames_manager.h"

#include "../firewallcontroller.h"
#include "../logger.h"

IpHostnamesManager::IpHostnamesManager(): isEnabled_(false)
{
    dnsResolver_.setResolveDomainsCallbackHandler(std::bind(&IpHostnamesManager::dnsResolverCallback, this, std::placeholders::_1));
}

IpHostnamesManager::~IpHostnamesManager()
{
    dnsResolver_.stop();
}

void IpHostnamesManager::enable(const std::string &gatewayIp)
{
    Logger::instance().out("IpHostnamesManager::enable(), begin");
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
    Logger::instance().out("IpHostnamesManager::enable(), end");
}

void IpHostnamesManager::disable()
{
    Logger::instance().out("IpHostnamesManager::disable(), begin");

    {
        std::lock_guard<std::recursive_mutex> guard(mutex_);

        if (!isEnabled_) {
            return;
        }
        ipRoutes_.clear();
        isEnabled_ = false;
    }
    dnsResolver_.cancelAll();
    FirewallController::instance().setSplitTunnelExceptions(std::vector<std::string>());
    Logger::instance().out("IpHostnamesManager::disable(), end");
}

void IpHostnamesManager::setSettings(const std::vector<std::string> &ips, const std::vector<std::string> &hosts)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    ipsLatest_ = ips;
    hostsLatest_ = hosts;
    Logger::instance().out("IpHostnamesManager::setSettings(), end");
}

void IpHostnamesManager::dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    std::vector<std::string> hostsIps;

    for (auto it = hostInfos.begin(); it != hostInfos.end(); ++it) {
        if (!it->second.error) {
            for (const auto addr : it->second.addresses) {
                if (addr == "0.0.0.0") {
                    // ROBERT sometimes will give us an address of 0.0.0.0 for a 'blocked' resource.  This is not a valid address.
                    Logger::instance().out("IpHostnamesManager::dnsResolverCallback(), Resolved : %s, IP: 0.0.0.0 (Ignored)", it->first.c_str());
                } else {
                    Logger::instance().out("IpHostnamesManager::dnsResolverCallback(), Resolved : %s, IP: %s", it->first.c_str(), addr.c_str());
                    hostsIps.insert(hostsIps.end(), addr);
                }
            }
        } else {
            Logger::instance().out("IpHostnamesManager::dnsResolverCallback(), Failed resolve : %s", it->first.c_str());
        }
    }

    hostsIps.insert(hostsIps.end(), ipsLatest_.begin(), ipsLatest_.end());

    if (isEnabled_) {
        ipRoutes_.setIps(gatewayIp_, hostsIps);
        FirewallController::instance().setSplitTunnelExceptions(hostsIps);
    }
}
