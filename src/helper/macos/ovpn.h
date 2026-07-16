#pragma once

#include <string>

namespace OVPN {

bool writeOVPNFile(const std::string &dnsScript, const std::string &config, const std::string &httpProxy,
                   unsigned int httpPort, const std::string &socksProxy, unsigned int socksPort,
                   const std::string &mgmtClientUser);

} //namespace OVPN
