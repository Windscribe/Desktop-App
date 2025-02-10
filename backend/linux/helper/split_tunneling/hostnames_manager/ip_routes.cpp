#include "ip_routes.h"

#include <set>
#include <spdlog/spdlog.h>
#include "../../utils.h"

void IpRoutes::setIps(const std::string &defaultRouteIp, const std::vector<std::string> &ips)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    // exclude duplicates
    std::set<std::string> ipsSet;
    for (auto ip = ips.begin(); ip != ips.end(); ++ip) {
        ipsSet.insert(*ip);
    }

    // find route which need to delete
    std::set<std::string> ipsDelete;
    for (auto it = activeRoutes_.begin(); it != activeRoutes_.end(); ++it) {
        if (it->second.defaultRouteIp != defaultRouteIp || ipsSet.find(it->first) == ipsSet.end()) {
            ipsDelete.insert(it->first);
        }
    }

    // delete routes
    for (auto ip = ipsDelete.begin(); ip != ipsDelete.end(); ++ip) {
        auto fr = activeRoutes_.find(*ip);
        if (fr != activeRoutes_.end()) {
            deleteRoute(fr->second);
            activeRoutes_.erase(fr);
        }
    }

    // add routes
    for (auto ip = ipsSet.begin(); ip != ipsSet.end(); ++ip) {
        auto ar = activeRoutes_.find(*ip);
        if (ar != activeRoutes_.end()) {
            continue;
        } else {
            RouteDescr rd;
            rd.ip = *ip;
            rd.defaultRouteIp = defaultRouteIp;
            addRoute(rd);
            activeRoutes_[*ip] = rd;
        }
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
    std::string cmd = "ip route add " + rd.ip + " via " + rd.defaultRouteIp;
    spdlog::info("cmd: {}", cmd);

    Utils::executeCommand(cmd);
}

void IpRoutes::deleteRoute(const RouteDescr &rd)
{
    std::string cmd = "ip route del " + rd.ip + " via " + rd.defaultRouteIp;
    spdlog::info("cmd: {}", cmd);
    Utils::executeCommand(cmd);
}
