#include "wg_dns_routes.h"

#include <algorithm>
#include <cstdint>
#include <sstream>

namespace WgDnsRoutes {

namespace {

std::string trimmed(const std::string &s)
{
    const auto first = s.find_first_not_of(" \t\r");
    if (first == std::string::npos) {
        return std::string();
    }
    const auto last = s.find_last_not_of(" \t\r");
    return s.substr(first, last - first + 1);
}

} // namespace

std::vector<types::IpAddress> serversNeedingTunnelRoute(const std::string &dnsAddressList,
                                                        const std::vector<std::string> &allowedIps,
                                                        const std::string &peerEndpoint)
{
    std::vector<types::IpAddressRange> ranges;
    for (const auto &allowedIp : allowedIps) {
        types::IpAddressRange range(allowedIp);
        if (range.isValid()) {
            ranges.push_back(range);
        }
    }

    std::string endpointHost;
    uint16_t endpointPort = 0;
    types::IpAddress endpoint;
    if (types::splitEndpoint(peerEndpoint, endpointHost, endpointPort)) {
        endpoint = types::IpAddress(endpointHost);
    }

    std::vector<types::IpAddress> result;
    std::string::size_type pos = 0;
    while (pos < dnsAddressList.size()) {
        const auto end = dnsAddressList.find_first_of(",; ", pos);
        const std::string token = dnsAddressList.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        pos = (end == std::string::npos) ? dnsAddressList.size() : end + 1;

        const types::IpAddress server(token);
        if (!server.isValid() || server.isLoopbackOrUnspecified() || (endpoint.isValid() && server == endpoint) ||
            std::find(result.begin(), result.end(), server) != result.end()) {
            continue;
        }
        const uint8_t hostPrefix = server.isV4() ? 32 : 128;
        bool covered = false;
        bool hasHostRoute = false;
        for (const auto &range : ranges) {
            if (range.contains(server)) {
                covered = true;
                hasHostRoute = hasHostRoute || range.prefixLength() == hostPrefix;
            }
        }
        if (covered && !hasHostRoute) {
            result.push_back(server);
        }
    }
    return result;
}

bool isDefaultRouteInMacRouteGet(const std::string &routeGetOutput)
{
    std::istringstream stream(routeGetOutput);
    std::string line;
    while (std::getline(stream, line)) {
        if (trimmed(line) == "mask: default") {
            return true;
        }
    }
    return false;
}

bool isDefaultOnlyInLinuxRouteMatch(const std::string &routeShowMatchOutput)
{
    std::istringstream stream(routeShowMatchOutput);
    std::string line;
    bool sawDefault = false;
    while (std::getline(stream, line)) {
        const std::string entry = trimmed(line);
        // Indented lines are the nexthops of a multipath route on the line above.
        if (entry.empty() || line[0] == ' ' || line[0] == '\t') {
            continue;
        }
        if (entry.rfind("default", 0) != 0) {
            return false;
        }
        sawDefault = true;
    }
    return sawDefault;
}

} // namespace WgDnsRoutes
