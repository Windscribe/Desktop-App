#include "validation_posix.h"

#include "helper_commands.h"

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <net/if.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/ip/address.hpp>
#include <skyr/core/parse.hpp>
#include <skyr/core/serialize.hpp>
#include <skyr/url.hpp>
#include <spdlog/spdlog.h>

namespace Validation {

bool isValidIpAddress(const std::string &ip)
{
    // Reject a scope-id suffix (e.g. "fe80::1%en0"): make_address parses it as valid, but callers
    // interpolate the result verbatim into pf/iptables tables and the OpenVPN config, none of which
    // accept "%zone" syntax. A scoped address must not be treated as a bare literal — a single one
    // reaching `pfctl -T load` rejects the whole <disallowed_dns> table and fails the kill switch.
    if (ip.find('%') != std::string::npos) {
        return false;
    }
    boost::system::error_code ec;
    boost::asio::ip::make_address(ip, ec);
    return !ec;
}

bool isValidIpv4Address(const std::string &ip)
{
    boost::system::error_code ec;
    boost::asio::ip::make_address_v4(ip, ec);
    return !ec;
}

bool isValidIpCidr(const std::string &ipCidr)
{
    size_t slash = ipCidr.find('/');

    std::string ipPart;
    if (slash == std::string::npos) {
        ipPart = ipCidr;
    } else {
        ipPart = ipCidr.substr(0, slash);
        std::string cidrPart = ipCidr.substr(slash + 1);

        // CIDR must be a non-empty numeric string
        if (cidrPart.empty()) {
            return false;
        }
        for (char c : cidrPart) {
            if (c < '0' || c > '9') {
                return false;
            }
        }

        errno = 0;
        long cidr = strtol(cidrPart.c_str(), nullptr, 10);
        if (errno != 0) {
            return false;
        }

        // Validate range after we know the address family
        boost::system::error_code ec;
        auto addr = boost::asio::ip::make_address(ipPart, ec);
        if (ec) {
            return false;
        }

        long maxCidr = addr.is_v4() ? 32 : 128;
        return cidr >= 0 && cidr <= maxCidr;
    }

    return isValidIpAddress(ipPart);
}

bool isValidIpv4Cidr(const std::string &ipCidr)
{
    size_t slash = ipCidr.find('/');
    if (slash == std::string::npos) {
        return isValidIpv4Address(ipCidr);
    }

    std::string ipPart = ipCidr.substr(0, slash);
    std::string cidrPart = ipCidr.substr(slash + 1);

    if (cidrPart.empty()) {
        return false;
    }
    for (char c : cidrPart) {
        if (c < '0' || c > '9') {
            return false;
        }
    }

    errno = 0;
    long cidr = strtol(cidrPart.c_str(), nullptr, 10);
    if (errno != 0 || cidr < 0 || cidr > 32) {
        return false;
    }

    return isValidIpv4Address(ipPart);
}

bool isValidPeerEndpoint(const std::string &endpoint)
{
    size_t colon = endpoint.rfind(':');
    if (colon == std::string::npos || colon == 0) {
        return false;
    }
    std::string host = endpoint.substr(0, colon);
    std::string port = endpoint.substr(colon + 1);

    if (port.empty()) {
        return false;
    }
    for (char c : port) {
        if (c < '0' || c > '9') {
            return false;
        }
    }
    errno = 0;
    long p = strtol(port.c_str(), nullptr, 10);
    if (errno != 0 || p < 1 || p > 65535) {
        return false;
    }

    boost::system::error_code ec;
    auto addr = boost::asio::ip::make_address(host, ec);
    return !ec && addr.is_v4();
}

bool isValidIpList(const std::string &addressList)
{
    if (addressList.empty()) {
        return true;
    }

    std::vector<std::string> parts;
    boost::split(parts, addressList, boost::is_any_of(",; "), boost::token_compress_on);

    for (const auto &part : parts) {
        if (part.empty()) {
            continue;
        }
        if (!isValidIpAddress(part)) {
            spdlog::warn("Validation: invalid IP address in list: \"{}\"", part);
            return false;
        }
    }

    return true;
}

bool isValidIpCidrList(const std::string &cidrList)
{
    if (cidrList.empty()) {
        return false;
    }

    std::vector<std::string> parts;
    boost::split(parts, cidrList, boost::is_any_of(",; "), boost::token_compress_on);

    bool any = false;
    for (const auto &part : parts) {
        if (part.empty()) {
            continue;
        }
        if (!isValidIpCidr(part)) {
            spdlog::warn("Validation: invalid CIDR in list: \"{}\"", part);
            return false;
        }
        any = true;
    }
    return any;
}

bool validateWireGuardConfig(const std::string &clientIpAddress,
                             const std::string &clientDnsAddressList,
                             const std::vector<std::string> &allowedIps,
                             const std::string &peerEndpoint)
{
    // clientIpAddress may be a comma-separated dual-stack list (v4 + v6) when
    // the WireGuard server pushes both families, e.g. "10.245.6.78/32, fd00:abcd::1/128".
    if (!isValidIpCidrList(clientIpAddress)) {
        spdlog::error("Validation: invalid client IP address: \"{}\"", clientIpAddress);
        return false;
    }

    if (!isValidIpList(clientDnsAddressList)) {
        spdlog::error("Validation: invalid DNS address list: \"{}\"", clientDnsAddressList);
        return false;
    }

    // AllowedIPs may legitimately contain IPv6 entries (e.g. "::/0", "fd00::/64") for
    // custom configs and dual-stack server pushes; isValidIpCidr accepts both families.
    for (const auto &ip : allowedIps) {
        if (!isValidIpCidr(ip)) {
            spdlog::error("Validation: invalid AllowedIP entry: \"{}\"", ip);
            return false;
        }
    }

    // peerEndpoint is intentionally restricted to IPv4: DefaultRouteMonitor on
    // Linux/macOS still hardcodes `ip route add <host>/32 via <gw>`, which would
    // fail for IPv6 hosts. Revisit if the monitor learns to handle IPv6 peers.
    if (!isValidPeerEndpoint(peerEndpoint)) {
        spdlog::error("Validation: invalid peer endpoint: \"{}\"", peerEndpoint);
        return false;
    }

    return true;
}

bool isValidDomain(const std::string &address)
{
    if (isValidIpv4Address(address)) {
        return false;
    }

    auto domain = skyr::parse_host(address);
    if (!domain) {
        return false;
    }

    return true;
}

bool isValidInterfaceName(const std::string &interfaceName)
{
    if (interfaceName.empty() || interfaceName.length() >= IFNAMSIZ) {
        return false;
    }

    // Whitelist the characters real interface names use (alphanumerics plus '.', '_', '-'). This is
    // intentionally stricter than "reject obvious separators": the name is interpolated verbatim
    // into iptables/pf rule text, so anything outside this set is rejected. In particular it bars
    // the iptables/pf wildcard '+' (which would match all interfaces and weaken the kill switch)
    // and pf grammar characters such as '}' (which could prematurely close an anchor block).
    for (unsigned char c : interfaceName) {
        if (!std::isalnum(c) && c != '.' && c != '_' && c != '-') {
            return false;
        }
    }

    return true;
}

bool isValidNetworkManagerConnectionName(const std::string &name)
{
    if (name.empty() || name.length() >= 256) {
        return false;
    }

    // A leading '-' would be parsed by nmcli as an option rather than a connection name.
    if (name.front() == '-') {
        return false;
    }

    // Reject control bytes (NUL/newline/tab/etc). Spaces and high (UTF-8) bytes are allowed, since
    // real NM connection names use them; argv-style execution prevents shell interpretation.
    for (unsigned char c : name) {
        if (c < 0x20 || c == 0x7f) {
            return false;
        }
    }

    return true;
}

void sanitizeFirewallConfig(FirewallConfig &config)
{
    if (!config.connectingIp.empty() && !isValidIpv4Address(config.connectingIp)) {
        spdlog::error("sanitizeFirewallConfig: invalid connectingIp \"{}\", dropping it", config.connectingIp);
        config.connectingIp.clear();
    }
    std::vector<std::string> validIps;
    validIps.reserve(config.allowedIps.size());
    for (const auto &ip : config.allowedIps) {
        if (isValidIpv4Address(ip)) {
            validIps.push_back(ip);
        } else {
            spdlog::error("sanitizeFirewallConfig: invalid allowedIp \"{}\", dropping it", ip);
        }
    }
    config.allowedIps = std::move(validIps);
    if (!config.vpnInterfaceName.empty() && !isValidInterfaceName(config.vpnInterfaceName)) {
        spdlog::error("sanitizeFirewallConfig: invalid vpnInterfaceName \"{}\", dropping it", config.vpnInterfaceName);
        config.vpnInterfaceName.clear();
    }
    std::vector<unsigned int> validPorts;
    validPorts.reserve(config.staticPorts.size());
    for (unsigned int port : config.staticPorts) {
        if (port > 0 && port <= 65535) {
            validPorts.push_back(port);
        } else {
            spdlog::error("sanitizeFirewallConfig: static port {} out of range, dropping it", port);
        }
    }
    config.staticPorts = std::move(validPorts);
}

bool isValidMacAddress(const std::string &macAddress)
{
    if (macAddress.empty()) {
        return false;
    }

    size_t len = macAddress.length();

    if (len == 12) {
        for (char c : macAddress) {
            if (!std::isxdigit(c)) {
                return false;
            }
        }
        return true;
    } else if (len == 17) {
        char separator = macAddress[2];
        if (separator != ':' && separator != '-') {
            return false;
        }

        for (size_t i = 0; i < len; i++) {
            if (i % 3 == 2) {
                if (macAddress[i] != separator) {
                    return false;
                }
            } else {
                if (!std::isxdigit(macAddress[i])) {
                    return false;
                }
            }
        }
        return true;
    }

    return false;
}

std::string normalizeAddress(const std::string &address)
{
    std::string addr = address;

    if (isValidIpv4Address(address)) {
        return addr;
    }

    if (isValidDomain(address)) {
        return addr;
    }

    auto url = skyr::parse(address);
    if (!url) {
        return "";
    }

    return skyr::serialize(url.value());
}

bool isPrintableSingleLineAscii(const std::string &v)
{
    for (unsigned char c : v) {
        if (c < 0x20 || c > 0x7E) {
            return false;
        }
    }
    return true;
}

} // namespace Validation
