#pragma once

#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/ip/address.hpp>
#include <spdlog/spdlog.h>

// Centralized IP/CIDR validation for WireGuard configuration fields.
// Validates that user-controlled strings (Address, DNS, AllowedIPs) contain
// only well-formed IP addresses before they reach shell commands or system calls.
// This prevents command injection via crafted .conf files.

namespace IpValidation {

// Validate a single IP address (no CIDR suffix).
// Returns true if the string is a valid IPv4 or IPv6 address.
inline bool isValidIpAddress(const std::string &ip)
{
    boost::system::error_code ec;
    boost::asio::ip::make_address(ip, ec);
    return !ec;
}

// Validate an IP/CIDR string (e.g. "10.0.0.1/24", "fd00::1/128").
// Accepts bare IPs without a CIDR suffix (treated as /32 or /128).
// Returns true if the IP portion is valid and the CIDR prefix length is
// a number in the correct range for the address family.
inline bool isValidIpCidr(const std::string &ipCidr)
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

// Validate a WireGuard peer endpoint of the form "host:port".
// The host portion must be a literal IPv4 address — DefaultRouteMonitor
// uses it directly in `ip route add <host>/32` (Linux) and `route ... -inet <host>/32`
// (macOS), so hostnames and IPv6 would fail those route commands regardless.
inline bool isValidPeerEndpoint(const std::string &endpoint)
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

// Validate a comma/semicolon/space-separated list of IP addresses (no CIDR).
// Used for DNS server lists.
inline bool isValidIpList(const std::string &addressList)
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
            spdlog::warn("IpValidation: invalid IP address in list: \"{}\"", part);
            return false;
        }
    }

    return true;
}

// Validate all WireGuard configuration fields that will be used in shell commands.
// Call this once before configure() and configureAdapter() in process_command.cpp.
// Returns true if all fields are safe to use.
inline bool validateWireGuardConfig(const std::string &clientIpAddress,
                                    const std::string &clientDnsAddressList,
                                    const std::vector<std::string> &allowedIps,
                                    const std::string &peerEndpoint)
{
    if (!isValidIpCidr(clientIpAddress)) {
        spdlog::error("IpValidation: invalid client IP address: \"{}\"", clientIpAddress);
        return false;
    }

    if (!isValidIpList(clientDnsAddressList)) {
        spdlog::error("IpValidation: invalid DNS address list: \"{}\"", clientDnsAddressList);
        return false;
    }

    for (const auto &ip : allowedIps) {
        if (!isValidIpCidr(ip)) {
            spdlog::error("IpValidation: invalid AllowedIP entry: \"{}\"", ip);
            return false;
        }
    }

    if (!isValidPeerEndpoint(peerEndpoint)) {
        spdlog::error("IpValidation: invalid peer endpoint: \"{}\"", peerEndpoint);
        return false;
    }

    return true;
}

} // namespace IpValidation
