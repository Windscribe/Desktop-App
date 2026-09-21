#pragma once

#include <WinSock2.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>

#include <map>
#include <mutex>
#include <vector>

#include "types/ipaddress.h"

// Manages a set of system routes for split-tunneling host pinning.
// Dual-stack (IPv4 + IPv6) — uses the modern netioapi.h API (MIB_IPFORWARD_ROW2 /
// *IpForwardEntry2). Family is dispatched per route based on the destination's family;
// IPv4 destinations use gatewayIp, IPv6 destinations use gatewayIpV6.
class IpRoutes
{
public:
    IpRoutes();

    // isExclude: in exclude mode a destination that is already directly attached over a live
    // interface is left to its own on-link route instead of being pinned to the gateway — see
    // collectOnLinkPrefixes(). Inclusive mode always installs the route, since the point there
    // is to pull the destination into the tunnel.
    void setIps(const types::IpAddress &gatewayIp,
                const types::IpAddress &gatewayIpV6,
                unsigned long ifIndex,
                const std::vector<types::IpAddressRange> &ips,
                bool isExclude);
    void clear();

private:
    std::recursive_mutex mutex_;
    std::map<types::IpAddressRange, MIB_IPFORWARD_ROW2> activeRoutes_;
};
