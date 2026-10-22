#pragma once

#include <string>
#include <vector>

namespace OVPN {

// Path of the --log file the helper reads when OpenVPN exits before binding its management socket.
inline const char *logPath()
{
    return WS_LINUX_RUN_DIR "/openvpn.log";
}

// Writes the filtered user config only. Helper-owned directives (management, up/down,
// dev, proxy, ...) are returned in extraArgs and must be passed on the openvpn command
// line after --config. --log is the exception: executeOpenVPN places it before --config
// so a fatal in the file is captured. The parser allowlist trusts the command line and
// rejects those names when they appear in the file.
bool writeOVPNFile(const std::string &dnsScript, const std::string &config, const std::string &httpProxy,
                   unsigned int httpPort, const std::string &socksProxy, unsigned int socksPort,
                   const std::string &mgmtClientUser, std::vector<std::string> &extraArgs);

void setUseDco(bool useDco);

} // namespace OVPN
