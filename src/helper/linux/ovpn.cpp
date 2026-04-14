#include "ovpn.h"
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "../common/ovpn_directive_whitelist.h"

namespace OVPN
{

bool writeOVPNFile(const std::string &dnsScript, unsigned int port, const std::string &config, const std::string &httpProxy,
                   unsigned int httpPort, const std::string &socksProxy, unsigned int socksPort)
{
    int fd = open(WS_POSIX_RUN_DIR "/config.ovpn", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
    if (fd < 0) {
        spdlog::error("Could not open config for writing");
        return false;
    }

    std::string filtered = OvpnDirectiveWhitelist::filterConfig(config,
        [](const std::string &line) { spdlog::warn("Blocked non-whitelisted OpenVPN directive: {}", line); });

    int bytes = write(fd, filtered.c_str(), filtered.length());
    if (bytes <= 0) {
        spdlog::error("Could not write openvpn config");
        close(fd);
        return false;
    }

    // add our own up/down scripts
    const std::string upScript = \
         "--script-security 2\n" \
         "up " + dnsScript + "\n" \
         "down " + dnsScript + "\n" \
         "down-pre\n" \
         "dhcp-option DOMAIN-ROUTE .\n"; // prevent DNS leakage and without it doesn't work update-systemd-resolved script
    bytes = write(fd, upScript.c_str(), upScript.length());
    if (bytes <= 0) {
        spdlog::error("Could not write openvpn config");
        close(fd);
        return false;
    }

    // add management and other options
    std::string opts = \
        "management 127.0.0.1 " + std::to_string(port) + "\n" \
        "management-query-passwords\n" \
        "management-hold\n" \
        "verb 3\n";

    if (httpProxy.length() > 0) {
        opts += "http-proxy " + httpProxy + " " + std::to_string(httpPort) + " auto\n";
    } else if (socksProxy.length() > 0) {
        opts += "socks-proxy " + socksProxy + " " + std::to_string(socksPort) + "\n";
    }

    bytes = write(fd, opts.c_str(), opts.length());
    if (bytes <= 0) {
        spdlog::error("Could not write additional options");
        close(fd);
        return false;
    }

    close(fd);
    return true;
}

} // namespace OVPN
