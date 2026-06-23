#include "ovpn.h"
#include <spdlog/spdlog.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>

#include "../common/io_posix.h"
#include "../common/ovpn_directive_whitelist.h"

namespace OVPN
{

bool writeOVPNFile(const std::string &dnsScript, unsigned int port, const std::string &config, const std::string &httpProxy,
                   unsigned int httpPort, const std::string &socksProxy, unsigned int socksPort)
{
    // open()'s mode is ignored on pre-existing files; unlink first to force the intended perms.
    // Unlinking before filterConfig also ensures a rejected config never leaves a stale file behind.
    unlink(WS_POSIX_CONFIG_DIR "/config.ovpn");

    std::string filtered;
    if (!OvpnDirectiveWhitelist::filterConfig(config, filtered)) {
        return false;
    }
    int fd = open(WS_POSIX_CONFIG_DIR "/config.ovpn", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        spdlog::error("Could not open config for writing");
        return false;
    }

    if (!IO::writeAll(fd, filtered)) {
        spdlog::error("Could not write openvpn config: {}", IO::strerror(errno));
        close(fd);
        return false;
    }

    // add our own up/down scripts
    const std::string upScript = \
        "--script-security 2\n" \
        "up \"" + dnsScript + " -up\"\n";
    if (!IO::writeAll(fd, upScript)) {
        spdlog::error("Could not write openvpn up script: {}", IO::strerror(errno));
        close(fd);
        return false;
    }

    // add management and other options
    std::string opts = \
        "dev tun\n" \
        "management 127.0.0.1 " + std::to_string(port) + "\n" \
        "management-query-passwords\n" \
        "management-hold\n" \
        "verb 3\n";

    if (httpProxy.length() > 0) {
        opts += "http-proxy " + httpProxy + " " + std::to_string(httpPort) + " auto\n";
    } else if (socksProxy.length() > 0) {
        opts += "socks-proxy " + socksProxy + " " + std::to_string(socksPort) + "\n";
    }

    if (!IO::writeAll(fd, opts)) {
        spdlog::error("Could not write additional options: {}", IO::strerror(errno));
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

} // namespace OVPN
