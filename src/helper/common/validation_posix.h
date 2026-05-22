#pragma once

#include <string>
#include <vector>

namespace Validation {

// Returns true if the string is a valid IPv4 or IPv6 address (no CIDR suffix).
bool isValidIpAddress(const std::string &ip);

// Returns true if the string is a valid IPv4 address (no CIDR suffix). Rejects IPv6.
// Use this where the address is interpolated into IPv4-specific contexts
// (unbracketed URLs, `route -n delete X/mask gw`, etc).
bool isValidIpv4Address(const std::string &ip);

// Returns true if the string is a valid IP/CIDR (e.g. "10.0.0.1/24", "fd00::1/128").
// Accepts bare IPs without a CIDR suffix. The prefix length must be in the correct
// range for the address family.
bool isValidIpCidr(const std::string &ipCidr);

// Returns true if the string is a valid IPv4/CIDR (e.g. "10.0.0.1/24"). Accepts bare
// IPv4 without a suffix. Rejects IPv6 — the WireGuard adapter on Linux/macOS runs IPv4-
// only iptables/ifconfig/route commands against these values.
bool isValidIpv4Cidr(const std::string &ipCidr);

// Returns true if the string is a valid WireGuard peer endpoint of the form "host:port".
// The host must be a literal IPv4 address — DefaultRouteMonitor uses it directly in
// `ip route add <host>/32` (Linux) / `route ... -inet <host>/32` (macOS), so hostnames
// and IPv6 would fail those route commands regardless.
bool isValidPeerEndpoint(const std::string &endpoint);

// Returns true if every entry in a comma/semicolon/space-separated list is a valid IP
// address (no CIDR). Empty list is considered valid. Used for DNS server lists.
bool isValidIpList(const std::string &addressList);

// Returns true if every entry in a comma/semicolon/space-separated list is a valid
// IP/CIDR (v4 or v6). Requires at least one valid entry. Used for the WireGuard
// client address, which may be dual-stack, e.g. "10.245.6.78/32, fd00:abcd::1/128".
bool isValidIpCidrList(const std::string &cidrList);

// Validate every WireGuard configuration field that will be used in shell commands.
// Call this once before configure() / configureAdapter() in process_command.cpp.
bool validateWireGuardConfig(const std::string &clientIpAddress,
                             const std::string &clientDnsAddressList,
                             const std::vector<std::string> &allowedIps,
                             const std::string &peerEndpoint);

// Returns true if the string is a valid hostname/domain (and not an IP address).
bool isValidDomain(const std::string &address);

// Returns true if the string is a valid network interface name
// (non-empty, shorter than IFNAMSIZ, no NUL/':'/'/' or whitespace).
bool isValidInterfaceName(const std::string &interfaceName);

// Returns true if the string is a valid MAC address in either
// 12-hex-digit form (e.g. "001122aabbcc") or 17-char form with ':' or '-'
// separators (e.g. "00:11:22:aa:bb:cc").
bool isValidMacAddress(const std::string &macAddress);

// Normalize an address string (IP, domain, or URL).
// Returns the address unchanged if it's a valid IPv4 or domain;
// otherwise attempts to parse and serialize as a URL.
// Returns "" if the input is none of the above.
std::string normalizeAddress(const std::string &address);

} // namespace Validation
