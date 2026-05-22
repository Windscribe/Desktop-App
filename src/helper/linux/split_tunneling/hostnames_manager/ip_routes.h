#pragma once

#include <map>
#include <mutex>
#include <vector>

#include "types/ipaddress.h"

// Manages a set of `ip route add` / `ip route del` entries used for split-tunnel
// host pinning. Dual-stack: each route's family is dispatched from the destination's
// family — v4 destinations are routed via gatewayV4, v6 destinations via gatewayV6.
// Either gateway may be invalid; destinations of an unavailable family are skipped
// with a warning.
class IpRoutes
{
public:
    void setIps(const types::IpAddress &gatewayV4,
                const types::IpAddress &gatewayV6,
                const std::vector<types::IpAddressRange> &ips);
    void clear();

private:
    std::recursive_mutex mutex_;

    struct RouteDescr
    {
        // Gateway the route was actually installed with, kept so we can detect a
        // gateway change between updates and reinstall affected routes.
        types::IpAddress defaultRouteIp;
        types::IpAddressRange ip;
    };

    // Keyed by destination range so v4/v6 entries don't collide on string equality
    // (e.g. `fd00::1` vs `fd00:0:0:0:0:0:0:1` would otherwise be tracked twice).
    std::map<types::IpAddressRange, RouteDescr> activeRoutes_;

    void addRoute(const RouteDescr &rd);
    void deleteRoute(const RouteDescr &rd);
};
