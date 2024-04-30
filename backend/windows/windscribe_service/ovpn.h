#pragma once

#include <string>

namespace OVPN {

bool writeOVPNFile(std::wstring &filename, int port, const std::wstring &config, const std::wstring &httpProxy, int httpPort, const std::wstring &socksProxy, int socksPort);

} //namespace OVPN