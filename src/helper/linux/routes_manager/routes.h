#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "types/ipaddress.h"

// helper for add and clear routes via system "ip route" command.
// Dual-stack: each entry stores the destination/gateway as types::IpAddress and
// the prefix length as a uint8_t. `ip route add` autodetects the family from the
// destination literal, so the same shell command shape works for both families.
class Routes
{
public:
    void add(const types::IpAddress &ip, const types::IpAddress &gateway, uint8_t prefixLength);
    void addWithInterface(const types::IpAddress &ip, const std::string &interface, uint8_t prefixLength);
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
