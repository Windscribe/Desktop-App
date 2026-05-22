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

bool writeOVPNFile(const std::string &dnsScript, unsigned int port, const std::string &config, const std::string &httpProxy,
                   unsigned int httpPort, const std::string &socksProxy, unsigned int socksPort)
{
    // open()'s mode is ignored on pre-existing files; unlink first to force the intended perms
    unlink(WS_LINUX_RUN_DIR "/config.ovpn");
    int fd = open(WS_LINUX_RUN_DIR "/config.ovpn", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        spdlog::error("Could not open config for writing");
        return false;
    }

    std::string filtered = OvpnDirectiveWhitelist::filterConfig(rewriteForDco(config),
        [](const std::string &name) { spdlog::warn("Blocked non-whitelisted OpenVPN directive: {}", name); },
        [](const std::string &name) { spdlog::info("Ignored OpenVPN directive: {}", name); });

    if (!IO::writeAll(fd, filtered)) {
        spdlog::error("Could not write openvpn config: {}", IO::strerror(errno));
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
    if (!IO::writeAll(fd, upScript)) {
        spdlog::error("Could not write openvpn config: {}", IO::strerror(errno));
        close(fd);
        return false;
    }

    // add management and other options
    std::string opts = \
        "management 127.0.0.1 " + std::to_string(port) + "\n" \
        "management-query-passwords\n" \
        "management-hold\n" \
        "verb 3\n";

    if (!s_useDco) {
        opts += "disable-dco\n";
        spdlog::info("Appended disable-dco to OpenVPN config");
    }

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
