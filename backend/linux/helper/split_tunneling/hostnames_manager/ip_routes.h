#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <map>

// manage Ip route via "route add" and "route delete" commands
class IpRoutes
{
public:
    void setIps(const std::string &defaultRouteIp, const std::vector<std::string> &ips);
    void clear();

private:
    std::recursive_mutex mutex_;

    struct RouteDescr
    {
        std::string defaultRouteIp;
        std::string ip;
    };

    std::map<std::string, RouteDescr> activeRoutes_;

    void addRoute(const RouteDescr &rd);
    void deleteRoute(const RouteDescr &rd);
};
