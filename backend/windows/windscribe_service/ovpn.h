#pragma once

#include <string>

namespace OVPN {

bool writeOVPNFile(std::wstring &filename, unsigned int port, const std::wstring &config, const std::wstring &httpProxy, unsigned int httpPort, const std::wstring &socksProxy, unsigned int socksPort);

} //namespace OVPN
