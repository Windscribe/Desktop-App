#pragma once

#include <string>

#include "types/ipaddress.h"


// macOS bound-route helper: pins traffic to a specific adapter via
// `route add -net 0.0.0.0 <gw> -ifscope <iface>` (v4) or
// `route add -inet6 -net :: <gw> -ifscope <iface>` (v6). One instance handles
// a single gateway; callers use two instances (boundRoute_ + boundRouteV6_) when
// they need to pin both families. Invalid IpAddress is logged and skipped.
class BoundRoute
{
public:
    BoundRoute();
    ~BoundRoute();

    void create(const types::IpAddress &ipAddress, const std::string &interfaceName);
    void remove();

private:
    bool isBoundRouteAdded_;
    types::IpAddress ipAddress_;
    std::string interfaceName_;
};
