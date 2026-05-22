#include "ip_routes.h"

#include <set>
#include <spdlog/spdlog.h>
#include "../../utils.h"

void IpRoutes::setIps(const types::IpAddress &gatewayV4,
                      const types::IpAddress &gatewayV6,
                      const std::vector<types::IpAddressRange> &ips)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    // Deduplicate input and drop any invalid entries up front.
    std::set<types::IpAddressRange> ipsSet;
    for (const auto &ip : ips) {
        if (ip.isValid()) {
            ipsSet.insert(ip);
        }
    }

    // Find routes that are no longer wanted, or whose family-specific gateway changed
    // since they were installed — both cases need a delete-then-(maybe)-add cycle.
    std::set<types::IpAddressRange> ipsDelete;
    for (auto it = activeRoutes_.begin(); it != activeRoutes_.end(); ++it) {
        const auto &rd = it->second;
        const auto &gwForFamily = rd.ip.isV6() ? gatewayV6 : gatewayV4;
        if (rd.defaultRouteIp != gwForFamily || ipsSet.find(it->first) == ipsSet.end()) {
            ipsDelete.insert(it->first);
        }
    }

    for (auto it = ipsDelete.begin(); it != ipsDelete.end(); ++it) {
        auto fr = activeRoutes_.find(*it);
        if (fr != activeRoutes_.end()) {
            deleteRoute(fr->second);
            activeRoutes_.erase(fr);
        }
    }

    for (auto it = ipsSet.begin(); it != ipsSet.end(); ++it) {
        if (activeRoutes_.find(*it) != activeRoutes_.end()) {
            continue;
        }

        // Pick the gateway by destination family; skip when the family's gateway is
        // not configured (OpenVPN-on-Linux is v4-only, so v6 destinations end up here
        // when inclusive-mode VPN is OpenVPN).
        const auto &gw = it->isV6() ? gatewayV6 : gatewayV4;
        if (!gw.isValid()) {
            spdlog::warn("IpRoutes::setIps(), no {} gateway for destination {}",
                         it->isV6() ? "IPv6" : "IPv4", it->toString());
            continue;
        }

        RouteDescr rd;
        rd.ip = *it;
        rd.defaultRouteIp = gw;
        addRoute(rd);
        activeRoutes_[*it] = rd;
    }
}

void IpRoutes::clear()
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    for (auto it = activeRoutes_.begin(); it != activeRoutes_.end(); ++it) {
        deleteRoute(it->second);
    }
    activeRoutes_.clear();
}

void IpRoutes::addRoute(const RouteDescr &rd)
{
    // `ip route add` autodetects family from the destination literal, so the same
    // command shape works for v4 and v6 — provided we paired the right gateway.
    const std::string dst = rd.ip.toString();
    const std::string via = rd.defaultRouteIp.toString();
    spdlog::info("cmd: ip route add {} via {}", dst, via);
    Utils::executeCommand("ip", {"route", "add", dst, "via", via});
}

void IpRoutes::deleteRoute(const RouteDescr &rd)
{
    const std::string dst = rd.ip.toString();
    const std::string via = rd.defaultRouteIp.toString();
    spdlog::info("cmd: ip route del {} via {}", dst, via);
    Utils::executeCommand("ip", {"route", "del", dst, "via", via});
}
