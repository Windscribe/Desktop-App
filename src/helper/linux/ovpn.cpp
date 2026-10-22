#include "ovpn.h"
#include <algorithm>
#include <fcntl.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <spdlog/spdlog.h>

#include "../common/io_posix.h"
#include "../common/ovpn_directive_whitelist.h"

namespace OVPN
{

static bool s_useDco = true;

void setUseDco(bool useDco)
{
    s_useDco = useDco;
}

static bool isDcoCipher(const std::string &c)
{
    return boost::iequals(c, "AES-128-GCM") || boost::iequals(c, "AES-256-GCM") || boost::iequals(c, "CHACHA20-POLY1305");
}

static std::string rewriteCipherListForDco(const std::string &value)
{
    if (!s_useDco) {
        return value;
    }
    std::vector<std::string> tokens;
    boost::split(tokens, value, boost::is_any_of(":"));
    if (!std::any_of(tokens.begin(), tokens.end(), isDcoCipher)) {
        return value;
    }
    tokens.erase(std::remove_if(tokens.begin(), tokens.end(),
        [](const std::string &t) { return boost::iends_with(t, "-CBC"); }),
        tokens.end());
    return boost::algorithm::join(tokens, ":");
}

static std::string rewriteForDco(const std::string &config)
{
    std::string out;
    std::istringstream stream(config);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        size_t directiveStart = 0;
        while (directiveStart < line.size() && (line[directiveStart] == ' ' || line[directiveStart] == '\t')) {
            ++directiveStart;
        }
        size_t directiveEnd = directiveStart;
        while (directiveEnd < line.size() && line[directiveEnd] != ' ' && line[directiveEnd] != '\t') {
            ++directiveEnd;
        }
        const size_t directiveLen = directiveEnd - directiveStart;
        const bool isCipherDirective =
            line.compare(directiveStart, directiveLen, "data-ciphers") == 0 ||
            line.compare(directiveStart, directiveLen, "ncp-ciphers") == 0;
        if (isCipherDirective) {
            size_t valStart = directiveEnd;
            while (valStart < line.size() && (line[valStart] == ' ' || line[valStart] == '\t')) {
                ++valStart;
            }
            const std::string leading = line.substr(0, directiveStart);
            const std::string whitespace = line.substr(directiveEnd, valStart - directiveEnd);
            const std::string value = (valStart < line.size()) ? rewriteCipherListForDco(line.substr(valStart)) : std::string();
            line = leading + "data-ciphers" + whitespace + value;
        }
        out += line;
        out += '\n';
    }
    return out;
}

bool writeOVPNFile(const std::string &dnsScript, const std::string &config, const std::string &httpProxy,
                   unsigned int httpPort, const std::string &socksProxy, unsigned int socksPort,
                   const std::string &mgmtClientUser, std::vector<std::string> &extraArgs)
{
    // open()'s mode is ignored on pre-existing files; unlink first to force the intended perms.
    // Unlinking before filterConfig also ensures a rejected config never leaves a stale file behind.
    unlink(WS_LINUX_RUN_DIR "/config.ovpn");
    // --log truncates, and unlinking first keeps the file 0600 even if a previous run widened it.
    unlink(logPath());

    // Remove any stale management socket so OpenVPN re-creates it fresh. The socket lives in the
    // root-owned run dir, so only root can create it here — a non-root process cannot pre-create it
    // to impersonate OpenVPN's management interface.
    unlink(WS_OVPN_MGMT_SOCKET);

    std::string filtered;
    if (!OvpnDirectiveWhitelist::filterConfig(rewriteForDco(config), filtered)) {
        return false;
    }

    int fd = open(WS_LINUX_RUN_DIR "/config.ovpn", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
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
    // The management socket is a unix domain socket in the root-owned run dir (not a loopback
    // TCP port), which removes the local port-grab/impersonation attack surface entirely.
    extraArgs = {
        "--script-security", "2",
        "--up", dnsScript,
        "--down", dnsScript,
        "--down-pre",
        // prevent DNS leakage; without it update-systemd-resolved does not work
        "--dhcp-option", "DOMAIN-ROUTE", ".",
        "--dev", "tun",
        "--management", WS_OVPN_MGMT_SOCKET, "unix",
        "--management-query-passwords",
        "--management-hold",
        "--verb", "3",
    };

    // Restrict the management interface to the specific user running the client. OpenVPN verifies the
    // connecting peer's credentials (getpeereid) and rejects any other uid, so even if the socket file
    // perms were somehow widened, only this user's client can drive the management interface. The
    // username is derived from the client's kernel-reported uid (SO_PEERCRED), never from client input.
    if (!mgmtClientUser.empty()) {
        extraArgs.push_back("--management-client-user");
        extraArgs.push_back(mgmtClientUser);
    }

    if (!s_useDco) {
        extraArgs.push_back("--disable-dco");
        spdlog::info("Passing --disable-dco on the OpenVPN command line");
    }

    if (httpProxy.length() > 0) {
        extraArgs.insert(extraArgs.end(), {"--http-proxy", httpProxy, std::to_string(httpPort), "auto"});
    } else if (socksProxy.length() > 0) {
        extraArgs.insert(extraArgs.end(), {"--socks-proxy", socksProxy, std::to_string(socksPort)});
    }

    return true;
}

} // namespace OVPN
