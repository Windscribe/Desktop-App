#include "hostnames_manager.h"

#include <spdlog/spdlog.h>
#include "../../firewallcontroller.h"

HostnamesManager::HostnamesManager(): isEnabled_(false),
    dnsResolver_(std::bind(&HostnamesManager::dnsResolverCallback, this, std::placeholders::_1)),
    routeChangeMonitor_(std::bind(&HostnamesManager::onRoutesChanged, this), IpRoutes::kRouteProtocol)
{
}

HostnamesManager::~HostnamesManager()
{
    routeChangeMonitor_.stop();
}

void HostnamesManager::enable(const types::IpAddress &gatewayIp,
                              const types::IpAddress &gatewayIpV6,
                              const types::IpAddress &adapterIp,
                              const types::IpAddress &adapterIpV6,
                              const std::string &adapterName,
                              const std::string &adapterNameV6,
                              const std::vector<std::string> &tunnelDevs)
{
    {
        std::lock_guard<std::recursive_mutex> guard(mutex_);

        gatewayIp_ = gatewayIp;
        gatewayIpV6_ = gatewayIpV6;
        adapterIp_ = adapterIp;
        adapterIpV6_ = adapterIpV6;
        adapterName_ = adapterName;
        adapterNameV6_ = adapterNameV6;
        tunnelDevs_ = tunnelDevs;
        currentIps_ = ipsLatest_;
        ipRoutes_.clear();
        ipRoutes_.setIps(gatewayIp_, gatewayIpV6_, adapterIp_, adapterIpV6_, adapterName_, adapterNameV6_,
                         tunnelDevs_, currentIps_);
        // Connect path: setSplitTunnelingEnabled() (and the resolveDomains callback below) apply right
        // after, so defer to avoid a wasted intermediate transaction.
        FirewallController::instance().setSplitTunnelIpExceptions(ipsLatest_, /*applyNow=*/false);
        isEnabled_ = true;
    }

    // Outside the lock: stop() joins the monitor thread, whose callback takes it. Only exclude mode
    // depends on the main table; inclusive pins go into the tunnel whatever it says.
    if (!tunnelDevs.empty()) {
        routeChangeMonitor_.start();
    } else {
        routeChangeMonitor_.stop();
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
    routeChangeMonitor_.stop();
    dnsResolver_.cancelAll();
    // Connect path: setSplitTunnelingEnabled() applies right after in setConnectParams, so defer.
    FirewallController::instance().setSplitTunnelIpExceptions(std::vector<types::IpAddressRange>(), /*applyNow=*/false);
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
        currentIps_ = hostsIps;
        ipRoutes_.setIps(gatewayIp_, gatewayIpV6_, adapterIp_, adapterIpV6_, adapterName_, adapterNameV6_,
                         tunnelDevs_, currentIps_);
        // Async path (not followed by setSplitTunnelingEnabled), so apply now (the default).
        FirewallController::instance().setSplitTunnelIpExceptions(hostsIps);
    }
}

void HostnamesManager::onRoutesChanged()
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    // A secondary interface coming up, losing its address or gaining a route changes which excluded
    // destinations the main table already covers, and lets a pin refused earlier (its gateway was
    // unreachable) go in now. The destination set is unchanged, so the firewall exceptions are too.
    if (isEnabled_ && !tunnelDevs_.empty()) {
        ipRoutes_.retryFailed();
        ipRoutes_.setIps(gatewayIp_, gatewayIpV6_, adapterIp_, adapterIpV6_, adapterName_, adapterNameV6_,
                         tunnelDevs_, currentIps_);
    }
}
