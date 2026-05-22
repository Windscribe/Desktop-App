#pragma once

#include <string>

#include "types/ipaddress.h"

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
