#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "types/ipaddress.h"


// Helper for adding and clearing routes via the system "route" command. Dual-stack: each
// entry stores destination/gateway as types::IpAddress and the prefix length as a uint8_t,
// and the family flag ("-inet" / "-inet6") is dispatched off the address family. CIDR
// destinations are passed as "<addr>/<prefix>"; macOS `route` accepts that form for both
// families.
class Routes
{
public:
    void add(const types::IpAddress &ip, const types::IpAddress &gateway, uint8_t prefixLength);
    void addWithInterface(const types::IpAddress &ip, uint8_t prefixLength, const std::string &interface);
    // Convenience overload for literal CIDRs (0.0.0.0/1, 128.0.0.0/1, ::/1, 8000::/1).
    void addWithInterface(const types::IpAddressRange &range, const std::string &interface);
    void clear();

private:
    struct RouteDescr
    {
        types::IpAddress ip;
        types::IpAddress gateway;
        uint8_t prefixLength = 0;
        std::string interface;
    };

    std::vector<RouteDescr> routes_;
};
