#include "ovpn.h"
#include <spdlog/spdlog.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "../common/io_posix.h"
#include "../common/ovpn_directive_whitelist.h"

namespace OVPN
{

bool writeOVPNFile(const std::string &dnsScript, const std::string &config, const std::string &httpProxy,
                   unsigned int httpPort, const std::string &socksProxy, unsigned int socksPort,
                   const std::string &mgmtClientUser, std::vector<std::string> &extraArgs)
{
    // open()'s mode is ignored on pre-existing files; unlink first to force the intended perms.
    // Unlinking before filterConfig also ensures a rejected config never leaves a stale file behind.
    unlink(WS_POSIX_CONFIG_DIR "/config.ovpn");
    // --log truncates, and unlinking first keeps the file 0600 even if a previous run widened it.
    unlink(logPath());

    // Remove any stale management socket so OpenVPN re-creates it fresh. The socket lives in the
    // root-owned config dir, so only root can create it here — a non-root process cannot pre-create
    // it to impersonate OpenVPN's management interface.
    unlink(WS_OVPN_MGMT_SOCKET);

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
    close(fd);

    // OpenVPN runs as root and appends. Creating the file here fixes the mode at 0600; --log truncates.
    const int logFd = open(logPath(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (logFd < 0) {
        spdlog::error("Could not create OpenVPN log: {}", IO::strerror(errno));
        return false;
    }
    close(logFd);

    // Helper-owned directives go on the command line, after --config. --log is placed before
    // --config by executeOpenVPN. The parser allowlist rejects these names in the config file,
    // which is the file the client can influence.
    // --up takes one parameter, and OpenVPN re-splits it with argv_parse_cmd(), so the path
    // (which lives under "Application Support") needs its own single quotes inside that one
    // argument.
    // The management socket is a unix domain socket in the root-owned config dir (not a loopback
    // TCP port), which removes the local port-grab/impersonation attack surface entirely.
    extraArgs = {
        "--script-security", "2",
        "--up", "'" + dnsScript + "' -up",
        "--dev", "tun",
        "--management", WS_OVPN_MGMT_SOCKET, "unix",
        "--management-query-passwords",
        "--management-hold",
        "--verb", "3",
    };

    // Restrict the management interface to the specific user running the client. OpenVPN verifies the
    // connecting peer's credentials (getpeereid) and rejects any other uid, so even if the socket file
    // perms were somehow widened, only this user's client can drive the management interface. The
    // username is derived from the client's kernel-reported uid (XPC euid), never from client input.
    if (!mgmtClientUser.empty()) {
        extraArgs.push_back("--management-client-user");
        extraArgs.push_back(mgmtClientUser);
    }

    if (httpProxy.length() > 0) {
        extraArgs.insert(extraArgs.end(), {"--http-proxy", httpProxy, std::to_string(httpPort), "auto"});
    } else if (socksProxy.length() > 0) {
        extraArgs.insert(extraArgs.end(), {"--socks-proxy", socksProxy, std::to_string(socksPort)});
    }

    return true;
}

} // namespace OVPN
