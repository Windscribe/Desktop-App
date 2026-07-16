#include "hostnames_manager.h"

#include <spdlog/spdlog.h>

#include "../callout_filter.h"
#include "../../firewallfilter.h"
#include "../../ipv6_firewall.h"

HostnamesManager::HostnamesManager(CalloutFilter *calloutFilter):
    calloutFilter_(calloutFilter), isEnabled_(false), isExcludeMode_(true), ifIndex_(0),
    dnsResolver_(std::bind(&HostnamesManager::dnsResolverCallback, this, std::placeholders::_1))
{
}

HostnamesManager::~HostnamesManager()
{
}

void HostnamesManager::enable(const types::IpAddress &gatewayIp,
                              const types::IpAddress &gatewayIpV6,
                              unsigned long ifIndex)
{
    spdlog::debug("HostnamesManager::enable(), begin");
    {
        std::lock_guard<std::recursive_mutex> guard(mutex_);

        gatewayIp_ = gatewayIp;
        gatewayIpV6_ = gatewayIpV6;
        ifIndex_ = ifIndex;
        ipRoutes_.clear();
        ipRoutes_.setIps(gatewayIp_, gatewayIpV6_, ifIndex_, ipsLatest_);
        FirewallFilter::instance().setSplitTunnelingWhitelistIps(ipsLatest_);
        // Mirror to CalloutFilter (see setV6WhitelistIps for the mode-specific v6 handling)
        // so manual IP/range entries apply before DNS resolution finishes; resolved
        // hostnames will replace this list in the next dnsResolverCallback().
        if (calloutFilter_) {
            calloutFilter_->setV6WhitelistIps(ipsLatest_);
        }
        Ipv6Firewall::instance().setSplitTunnelingWhitelistIps(ipsLatest_);
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

        if (!isEnabled_)
        {
            return;
        }
        ipRoutes_.clear();
        isEnabled_ = false;
    }
    dnsResolver_.cancelAll();
    spdlog::debug("HostnamesManager::disable(), end");
}

void HostnamesManager::setSettings(bool isExclude,
                                   const std::vector<types::IpAddressRange> &ips,
                                   const std::vector<std::string> &hosts)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    ipsLatest_ = ips;
    hostsLatest_ = hosts;
    isExcludeMode_ = isExclude;
    spdlog::debug("HostnamesManager::setSettings(), end");
}

void HostnamesManager::dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    std::vector<types::IpAddressRange> hostsIps;
    for (auto it = hostInfos.begin(); it != hostInfos.end(); ++it) {
        if (it->second.error) {
            spdlog::warn("HostnamesManager::dnsResolverCallback(), Failed resolve : {}", it->first);
            continue;
        }
        for (const auto &addr : it->second.addresses) {
            // ROBERT sometimes returns 0.0.0.0 for a 'blocked' resource — filter it out.
            if (addr == "0.0.0.0") {
                spdlog::debug("HostnamesManager::dnsResolverCallback(), Resolved : {}, IP: 0.0.0.0 (Ignored)", it->first);
                continue;
            }
            // Build a single-host range. Family is auto-detected from the literal,
            // prefix length defaults to /32 (v4) or /128 (v6).
            types::IpAddress parsed(addr);
            if (!parsed.isValid()) {
                spdlog::warn("HostnamesManager::dnsResolverCallback(), unparsable address {} for {}", addr, it->first);
                continue;
            }
            uint8_t prefix = parsed.isV6() ? 128 : 32;
            hostsIps.emplace_back(parsed, prefix);
            spdlog::debug("HostnamesManager::dnsResolverCallback(), Resolved : {}, IP: {}", it->first, addr);
        }
    }

    hostsIps.insert(hostsIps.end(), ipsLatest_.begin(), ipsLatest_.end());

    if (isEnabled_) {
        ipRoutes_.setIps(gatewayIp_, gatewayIpV6_, ifIndex_, hostsIps);
        FirewallFilter::instance().setSplitTunnelingWhitelistIps(hostsIps);
        if (calloutFilter_) {
            calloutFilter_->setV6WhitelistIps(hostsIps);
        }
        Ipv6Firewall::instance().setSplitTunnelingWhitelistIps(hostsIps);
    }
}
