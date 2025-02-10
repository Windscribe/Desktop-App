#include "ip_hostnames_manager.h"
#include <spdlog/spdlog.h>
#include "../firewallcontroller.h"

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
    spdlog::debug("IpHostnamesManager::enable(), begin");
    {
        std::lock_guard<std::mutex> guard(mutex_);

        gatewayIp_ = gatewayIp;
        ipRoutes_.clear();
        ipRoutes_.setIps(gatewayIp_, ipsLatest_);
        FirewallController::instance().setSplitTunnelExceptions(ipsLatest_);
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
        ipRoutes_.clear();
        isEnabled_ = false;
    }
    dnsResolver_.cancelAll();
    FirewallController::instance().setSplitTunnelExceptions(std::vector<std::string>());
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

    std::vector<std::string> hostsIps;

    for (auto it = hostInfos.begin(); it != hostInfos.end(); ++it) {
        if (!it->second.error) {
            for (const auto addr : it->second.addresses) {
                if (addr == "0.0.0.0") {
                    // ROBERT sometimes will give us an address of 0.0.0.0 for a 'blocked' resource.  This is not a valid address.
                    spdlog::debug("IpHostnamesManager::dnsResolverCallback(), Resolved : {}, IP: 0.0.0.0 (Ignored)", it->first);
                } else {
                    spdlog::debug("IpHostnamesManager::dnsResolverCallback(), Resolved : {}, IP: {}", it->first, addr);
                    hostsIps.insert(hostsIps.end(), addr);
                }
            }
        } else {
            spdlog::warn("IpHostnamesManager::dnsResolverCallback(), Failed resolve : {}", it->first);
        }
    }

    hostsIps.insert(hostsIps.end(), ipsLatest_.begin(), ipsLatest_.end());

    if (isEnabled_) {
        ipRoutes_.setIps(gatewayIp_, hostsIps);
        FirewallController::instance().setSplitTunnelExceptions(hostsIps);
    }
}
