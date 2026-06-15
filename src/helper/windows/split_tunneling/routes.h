#pragma once

#include <WinSock2.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>

#include <vector>

#include "types/ipaddress.h"

class IpForwardTable;

// Dual-stack (IPv4 + IPv6) route add/delete with revert-on-demand.
// Uses the modern netioapi.h API (MIB_IPFORWARD_ROW2 / *IpForwardEntry2).
class Routes
{
public:
    Routes();

    void deleteRoute(const IpForwardTable &curRouteTable,
                     const types::IpAddress &destIp, UINT8 prefixLength,
                     const types::IpAddress &gatewayIp, unsigned long ifIndex);

    void deleteRouteByInterface(const IpForwardTable &curRouteTable,
                                const types::IpAddress &destIp, UINT8 prefixLength,
                                unsigned long ifIndex);

    void addRoute(const IpForwardTable &curRouteTable,
                  const types::IpAddress &destIp, UINT8 prefixLength,
                  const types::IpAddress &gatewayIp, unsigned long ifIndex,
                  bool useMaxMetric);

    void revertRoutes();

private:
    std::vector<MIB_IPFORWARD_ROW2> deletedRoutes_;
    std::vector<MIB_IPFORWARD_ROW2> addedRoutes_;
};
