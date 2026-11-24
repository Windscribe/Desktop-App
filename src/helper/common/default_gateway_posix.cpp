#include "default_gateway_posix.h"

#include <sstream>
#include <vector>

#include "types/ipaddress.h"

namespace DefaultGateway {

namespace {

std::vector<std::string> fields(const std::string &line)
{
    std::istringstream stream(line);
    std::vector<std::string> result;
    std::string field;
    while (stream >> field) {
        result.push_back(field);
    }
    return result;
}

bool isIpv4(const std::string &address)
{
    const types::IpAddress ip(address);
    return ip.isValid() && ip.isV4();
}

} // namespace

std::string fromMacNetstat(const std::string &netstatOutput)
{
    std::istringstream stream(netstatOutput);
    std::string line;
    while (std::getline(stream, line)) {
        const auto parts = fields(line);
        if (parts.size() >= 2 && parts[0] == "default" && isIpv4(parts[1])) {
            return parts[1];
        }
    }
    return std::string();
}

std::string fromLinuxIpRoute(const std::string &ipRouteOutput)
{
    std::istringstream stream(ipRouteOutput);
    std::string line;
    while (std::getline(stream, line)) {
        const auto parts = fields(line);
        if (parts.size() >= 3 && parts[0] == "default" && parts[1] == "via" && isIpv4(parts[2])) {
            return parts[2];
        }
    }
    return std::string();
}

} // namespace DefaultGateway
