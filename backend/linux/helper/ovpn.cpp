#include "ovpn.h"
#include "logger.h"
#include <fcntl.h>
#include <sstream>
#include <string>
#include <unistd.h>

namespace OVPN
{

bool writeOVPNFile(const std::string &dnsScript, const std::string &config, bool isCustomConfig)
{
    std::istringstream stream(config);
    std::string line;
    int bytes;

    int fd = open("/etc/windscribe/config.ovpn", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
    if (fd < 0) {
        Logger::instance().out("Could not open firewall rules for writing");
        return false;
    }

    while(getline(stream, line)) {
        // trim whitespace
        line.erase(0, line.find_first_not_of(" \n\r\t"));
        line.erase(line.find_last_not_of(" \n\r\t") + 1);

        // filter anything that runs an external script
        // check for up to offset of 2 in case the command starts with '--'
        if (line.rfind("up", 2) != std::string::npos ||
            line.rfind("tls-verify", 2) != std::string::npos ||
            line.rfind("ipchange", 2) != std::string::npos ||
            line.rfind("client-connect", 2) != std::string::npos ||
            line.rfind("route-up", 2) != std::string::npos ||
            line.rfind("route-pre-down", 2) != std::string::npos ||
            line.rfind("client-disconnect", 2) != std::string::npos ||
            line.rfind("down", 2) != std::string::npos ||
            line.rfind("learn-address", 2) != std::string::npos ||
            line.rfind("auth-user-pass-verify", 2) != std::string::npos)
        {
            continue;
        }

        bytes = write(fd, (line + "\n").c_str(), line.length() + 1);
        if (bytes <= 0) {
            Logger::instance().out("Could not write openvpn config");
            close(fd);
            return false;
        }

    }

    // add our own up/down scripts
    if (!isCustomConfig) {
        const std::string upScript = \
            "--script-security 2\n" \
            "up " + dnsScript + "\n" \
            "down " + dnsScript + "\n" \
            "down-pre\n" \
            "dhcp-option DOMAIN-ROUTE .\n"; // prevent DNS leakage and without it doesn't work update-systemd-resolved script
        bytes = write(fd, upScript.c_str(), upScript.length());
        if (bytes <= 0) {
            Logger::instance().out("Could not write openvpn config");
            close(fd);
            return false;
        }
    }
    close(fd);
    return true;
}

} // namespace OVPN
