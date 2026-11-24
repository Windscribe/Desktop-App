#pragma once

#include <string>
#include <vector>

#include "types/ipaddress.h"

namespace WgDnsRoutes {

std::vector<types::IpAddress> serversNeedingTunnelRoute(const std::string &dnsAddressList,
                                                        const std::vector<std::string> &allowedIps,
                                                        const std::string &peerEndpoint);

bool isDefaultRouteInMacRouteGet(const std::string &routeGetOutput);
bool isDefaultOnlyInLinuxRouteMatch(const std::string &routeShowMatchOutput);

} // namespace WgDnsRoutes
