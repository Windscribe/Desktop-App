#include "ovpn_posix.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "io_posix.h"
#include "ovpn_directive_whitelist.h"

#if defined(__linux__)
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <sstream>

#include "../linux/tunnel_interface_names.h"
#endif

#if defined(__APPLE__)
#define WS_OVPN_FILE_DIR WS_POSIX_CONFIG_DIR
#else
#define WS_OVPN_FILE_DIR WS_LINUX_RUN_DIR
#endif

namespace OVPN
{

#if defined(__linux__)
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

#endif

bool writeOVPNFile(const std::string &dnsScript, const std::string &config, const std::string &httpProxy,
                   unsigned int httpPort, const std::string &socksProxy, unsigned int socksPort,
                   const std::string &mgmtClientUser, std::vector<std::string> &extraArgs)
{
    // open()'s mode is ignored on pre-existing files; unlink first to force the intended perms.
    // Unlinking before filterConfig also ensures a rejected config never leaves a stale file behind.
    unlink(WS_OVPN_FILE_DIR "/config.ovpn");
    // --log truncates, and unlinking first keeps the file 0600 even if a previous run widened it.
    unlink(logPath());

    // Remove any stale management socket so OpenVPN re-creates it fresh. The socket lives in the
    // root-owned directory, so only root can create it here — a non-root process cannot pre-create
    // it to impersonate OpenVPN's management interface.
    unlink(WS_OVPN_MGMT_SOCKET);

    std::string filtered;
#if defined(__linux__)
    if (!OvpnDirectiveWhitelist::filterConfig(rewriteForDco(config), filtered)) {
#else
    if (!OvpnDirectiveWhitelist::filterConfig(config, filtered)) {
#endif
        return false;
    }

    int fd = open(WS_OVPN_FILE_DIR "/config.ovpn", O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, S_IRUSR | S_IWUSR);
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
    const int logFd = open(logPath(), O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if (logFd < 0) {
        spdlog::error("Could not create OpenVPN log: {}", IO::strerror(errno));
        return false;
    }
    close(logFd);

    // Helper-owned directives go on the command line after --config (--log goes before it); the allowlist rejects them in the file.
    // OpenVPN re-splits the single --route-up argument, so the macOS path (under "Application Support") carries its own quotes.
    // The management socket is a unix socket in a root-owned dir, not a loopback TCP port another local process could grab.
    extraArgs = {
        "--script-security", "2",
        "--dns-updown", "disable",
        // Without an --up script, OpenVPN runs its own DNS script as root and withholds `dns` directive servers from ours.
#if defined(__APPLE__)
        "--up", "/usr/bin/true",
        "--route-up", "'" + dnsScript + "' -up",
#else
        "--up", "/bin/true",
        "--route-up", dnsScript,
        "--down", dnsScript,
        "--down-pre",
        // prevent DNS leakage; without it update-systemd-resolved does not work
        "--dhcp-option", "DOMAIN-ROUTE", ".",
#endif
#if defined(__APPLE__)
        "--dev", "tun",
#else
        // A fixed name, so NetworkManager's drop-in can leave it unmanaged (see tunnel_interface_names.h).
        // The "tun" prefix tells OpenVPN the device type, as the generic "tun" did.
        "--dev", TunnelInterfaceNames::kOpenVpn,
#endif
        "--management", WS_OVPN_MGMT_SOCKET, "unix",
        "--management-query-passwords",
        "--management-hold",
        "--verb", "3",
    };

    // Restrict the management interface to the specific user running the client. OpenVPN verifies the
    // connecting peer's credentials (getpeereid) and rejects any other uid, so even if the socket file
    // perms were somehow widened, only this user's client can drive the management interface. The
    // username is derived from the client's kernel-reported uid, never from client input.
    if (!mgmtClientUser.empty()) {
        extraArgs.push_back("--management-client-user");
        extraArgs.push_back(mgmtClientUser);
    }

#if defined(__linux__)
    if (!s_useDco) {
        extraArgs.push_back("--disable-dco");
        spdlog::info("Passing --disable-dco on the OpenVPN command line");
    }
#endif

    if (httpProxy.length() > 0) {
        extraArgs.insert(extraArgs.end(), {"--http-proxy", httpProxy, std::to_string(httpPort), "auto"});
    } else if (socksProxy.length() > 0) {
        extraArgs.insert(extraArgs.end(), {"--socks-proxy", socksProxy, std::to_string(socksPort)});
    }

    return true;
}

} // namespace OVPN
