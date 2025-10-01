#pragma once

#include <string>

namespace OVPN {

bool writeOVPNFile(const std::string &dnsScript, unsigned int port, const std::string &config, const std::string &httpProxy,
                   unsigned int httpPort, const std::string &socksProxy, unsigned int socksPort, bool isCustomConfig);

} // namespace OVPN
