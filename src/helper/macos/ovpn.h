#pragma once

#include <string>
#include <vector>

namespace OVPN {

// Path of the --log file the helper reads when OpenVPN exits before binding its management socket.
inline const char *logPath()
{
    return WS_POSIX_CONFIG_DIR "/openvpn.log";
}

// Writes the filtered user config only. Helper-owned directives are returned in extraArgs
// and must be passed on the openvpn command line after --config. --log is the exception:
// executeOpenVPN places it before --config so a fatal in the file is captured.
bool writeOVPNFile(const std::string &dnsScript, const std::string &config, const std::string &httpProxy,
                   unsigned int httpPort, const std::string &socksProxy, unsigned int socksPort,
                   const std::string &mgmtClientUser, std::vector<std::string> &extraArgs);

} //namespace OVPN
