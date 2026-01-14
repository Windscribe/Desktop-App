#pragma once

#include <windows.h>
#include <iphlpapi.h>

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "../../ip_address/ip4_address_and_mask.h"

class IpRoutes
{
public:
    IpRoutes();

    void setIps(const std::string gatewayIp, unsigned long ifIndex, const std::vector<Ip4AddressAndMask> &ips);
    void clear();

private:
    std::recursive_mutex mutex_;
    std::map<Ip4AddressAndMask, MIB_IPFORWARDROW> activeRoutes_;
};
