#include "hostnames_manager.h"

#include <spdlog/spdlog.h>
#include "../../firewallcontroller.h"

HostnamesManager::HostnamesManager(): isEnabled_(false),
    dnsResolver_(std::bind(&HostnamesManager::dnsResolverCallback, this, std::placeholders::_1))
{
}

HostnamesManager::~HostnamesManager()
{
}

void HostnamesManager::enable(const types::IpAddress &gatewayIp,
                              const types::IpAddress &gatewayIpV6,
                              const types::IpAddress &adapterIp,
                              const types::IpAddress &adapterIpV6,
                              const std::string &adapterName,
                              const std::string &adapterNameV6)
{
    {
        std::lock_guard<std::recursive_mutex> guard(mutex_);

        gatewayIp_ = gatewayIp;
        gatewayIpV6_ = gatewayIpV6;
        adapterIp_ = adapterIp;
        adapterIpV6_ = adapterIpV6;
        adapterName_ = adapterName;
        adapterNameV6_ = adapterNameV6;
        ipRoutes_.clear();
        ipRoutes_.setIps(gatewayIp_, gatewayIpV6_, adapterIp_, adapterIpV6_, adapterName_, adapterNameV6_, ipsLatest_);
        FirewallController::instance().setSplitTunnelIpExceptions(ipsLatest_);
        isEnabled_ = true;
    }

    dnsResolver_.cancelAll();
    dnsResolver_.resolveDomains(hostsLatest_);
}

void HostnamesManager::disable()
{
    {
        std::lock_guard<std::recursive_mutex> guard(mutex_);

        if (!isEnabled_) {
            return;
        }
        ipRoutes_.clear();
        isEnabled_ = false;
    }
    dnsResolver_.cancelAll();
    FirewallController::instance().setSplitTunnelIpExceptions(std::vector<types::IpAddressRange>());
}

void HostnamesManager::setSettings(const std::vector<types::IpAddressRange> &ips,
                                   const std::vector<std::string> &hosts)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    ipsLatest_ = ips;
    hostsLatest_ = hosts;
}

void HostnamesManager::dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    std::vector<types::IpAddressRange> hostsIps;
    for (auto it = hostInfos.begin(); it != hostInfos.end(); ++it) {
        if (it->second.error) {
            spdlog::debug("HostnamesManager::dnsResolverCallback(), Failed resolve : {}", it->first);
            continue;
        }
        for (const auto &addr : it->second.addresses) {
            // ROBERT returns 0.0.0.0 for a 'blocked' hostname — drop those.
            if (addr == "0.0.0.0") {
                spdlog::debug("HostnamesManager::dnsResolverCallback(), Resolved : {}, IP: 0.0.0.0 (Ignored)", it->first);
                continue;
            }
            types::IpAddress parsed(addr);
            if (!parsed.isValid()) {
                spdlog::warn("HostnamesManager::dnsResolverCallback(), unparsable address {} for {}", addr, it->first);
                continue;
            }
            // Single-host range: /32 for v4, /128 for v6.
            uint8_t prefix = parsed.isV6() ? 128 : 32;
            hostsIps.emplace_back(parsed, prefix);
            spdlog::debug("HostnamesManager::dnsResolverCallback(), Resolved : {}, IP: {}", it->first, addr);
        }
    }

    hostsIps.insert(hostsIps.end(), ipsLatest_.begin(), ipsLatest_.end());

    if (isEnabled_) {
        ipRoutes_.setIps(gatewayIp_, gatewayIpV6_, adapterIp_, adapterIpV6_, adapterName_, adapterNameV6_, hostsIps);
        FirewallController::instance().setSplitTunnelIpExceptions(hostsIps);
    }
}
