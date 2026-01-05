#include "ovpn.h"
#include <spdlog/spdlog.h>
#include <fcntl.h>
#include <sstream>
#include <string>
#include <unistd.h>

namespace OVPN
{

bool writeOVPNFile(const std::string &dnsScript, unsigned int port, const std::string &config, const std::string &httpProxy,
                   unsigned int httpPort, const std::string &socksProxy, unsigned int socksPort, bool isCustomConfig)
{
    std::istringstream stream(config);
    std::string line;
    int bytes;

    int fd = open("/etc/windscribe/config.ovpn", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
    if (fd < 0) {
        spdlog::error("Could not open config for writing");
        return false;
    }

    while(getline(stream, line)) {
        // trim whitespace
        line.erase(0, line.find_first_not_of(" \n\r\t"));
        line.erase(line.find_last_not_of(" \n\r\t") + 1);

        // filter anything that runs an external script, or we need to override
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
            line.rfind("auth-user-pass-verify", 2) != std::string::npos ||
            line.rfind("management", 2) != std::string::npos ||
            line.rfind("http-proxy", 2) != std::string::npos ||
            line.rfind("socks-proxy", 2) != std::string::npos)
        {
            continue;
        }

        bytes = static_cast<int>(write(fd, (line + "\n").c_str(), line.length() + 1));
        if (bytes <= 0) {
            spdlog::error("Could not write openvpn config");
            close(fd);
            return false;
        }

    }

    // add our own up/down scripts
    const std::string upScript = \
        "--script-security 2\n" \
        "up \"" + dnsScript + " -up\"\n";
    (void)write(fd, upScript.c_str(), upScript.length());

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

    bytes = static_cast<int>(write(fd, opts.c_str(), opts.length()));
    if (bytes <= 0) {
        spdlog::error("Could not write additional options");
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

} // namespace OVPN
