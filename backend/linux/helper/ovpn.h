#pragma once

#include <string>

namespace OVPN {

bool writeOVPNFile(const std::string &dnsScript, int port, const std::string &config, const std::string &httpProxy, int httpPort, const std::string &socksProxy, int socksPort, bool isCustomConfig);

} // namespace OVPN