#pragma once

#include <string>

namespace OVPN {

bool writeOVPNFile(const std::string &dnsScript, const std::string &config, bool isCustomConfig);

} // namespace OVPN