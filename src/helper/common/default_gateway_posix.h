#pragma once

#include <string>

namespace DefaultGateway {

// Each returns the first IPv4 default gateway in the command's output, or an empty string.
std::string fromMacNetstat(const std::string &netstatOutput);
std::string fromLinuxIpRoute(const std::string &ipRouteOutput);

} // namespace DefaultGateway
