#pragma once

#include <string>
#include <vector>

struct AmneziawgConfig;
struct FirewallConfig;

namespace Validation {

// Returns true if the string is a valid IPv4 or IPv6 address (no CIDR suffix, no scope-id suffix
// such as "fe80::1%en0"). Callers interpolate the result into pf/iptables/OpenVPN text that does
// not accept "%zone" syntax, so a scoped address is rejected rather than treated as a bare literal.
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

// Returns true if the string is a valid network interface name: non-empty, shorter than
// IFNAMSIZ, and composed only of [A-Za-z0-9._-]. The whitelist deliberately bars the
// iptables/pf wildcard '+' and pf grammar characters, since the name is interpolated into
// rule text the privileged helper applies.
bool isValidInterfaceName(const std::string &interfaceName);

// Returns true if the string is acceptable as a NetworkManager connection name to pass to
// `nmcli connection modify/up <name>`. NM names may legally contain spaces and UTF-8, so this
// is intentionally permissive: it rejects empty, names >= 256 bytes, a leading '-' (which nmcli
// would parse as an option), and control bytes (< 0x20 or 0x7F, covering NUL/newline/tab).
// Shell injection is already precluded by the argv-style executeCommand.
bool isValidNetworkManagerConnectionName(const std::string &name);

// Sanitizes the tokens of a FirewallConfig that the privileged helper will interpolate into
// iptables/pf rule text, in place. connectingIp and each allowedIps entry must be a valid IPv4
// literal (the helper only emits these as v4 allow-rules and appends /32); vpnInterfaceName must
// pass isValidInterfaceName; each staticPorts entry must be in the 1..65535 range. Invalid entries
// are dropped/cleared rather than rejecting the whole command, so the firewall still comes up
// from the valid remainder instead of leaving the firewall off and leaking. Shared by the Linux and
// macOS setFirewallRules handlers.
void sanitizeFirewallConfig(FirewallConfig &config);

// Returns true if the string is a valid MAC address in either
// 12-hex-digit form (e.g. "001122aabbcc") or 17-char form with ':' or '-'
// separators (e.g. "00:11:22:aa:bb:cc").
bool isValidMacAddress(const std::string &macAddress);

// Returns true if the string is a plausible system username: non-empty and free of
// whitespace and control characters. Intended for values that are interpolated into
// line- or token-oriented sinks where whitespace or control bytes would change the
// parse. For example, the OpenVPN "management-client-user" directive.
bool isValidUsername(const std::string &name);

// Returns true iff every byte is printable ASCII (0x20..0x7E). Rejects newlines,
// tabs, NULs, and all non-ASCII. Empty string is considered valid; callers that need
// to also reject empty values should check that separately. Used at boundaries that
// hand a value off to a line-oriented sink (AmneziaWG UAPI socket records) where a
// smuggled newline could be reinterpreted as a new record.
bool isPrintableSingleLineAscii(const std::string &v);

// Returns true if the value is non-empty and passes isPrintableSingleLineAscii; otherwise
// logs an error naming fieldName and returns false. Used for WireGuard UAPI key fields
// (private_key/public_key/preshared_key) before they are written as line-oriented records.
bool isValidUapiKeyField(const char *fieldName, const std::string &value);

// Returns true if every AmneziaWG obfuscation string (I-values and H1-H4) is empty or passes
// isPrintableSingleLineAscii, logging the offending field type on failure. Shared by the Linux and
// macOS WireGuard communicators before the values are written as newline-delimited UAPI records.
bool isValidAmneziawgObfuscationFields(const AmneziawgConfig &config);

// Normalize an address string (IP, domain, or URL).
// Returns the address unchanged if it's a valid IPv4 or domain;
// otherwise attempts to parse and serialize as a URL.
// Returns "" if the input is none of the above.
std::string normalizeAddress(const std::string &address);

} // namespace Validation
