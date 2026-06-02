#pragma once

#include <string>

namespace NetworkValidation
{
    bool isValidIpAddress(const std::wstring &ip);

    // Accepts a bare IPv4/IPv6 address or an "<ip>/<prefix>" CIDR form, where prefix
    // is in [0, 32] for IPv4 and [0, 128] for IPv6.
    bool isValidIpOrCidr(const std::wstring &value);

    // Strict RFC compliance: DnsNameHostnameFull rejects loose forms (e.g., underscore
    // labels) so whitespace and other characters that could break serialized contexts
    // cannot slip through.
    bool isValidHostname(const std::wstring &hostname);

    bool isMacAddress(const std::wstring &value);

    // Accepts a comma-separated list of TCP/UDP ports or port ranges (e.g.,
    // "80", "8000-8080", "80,443,1000-2000"). Each value must be in [1, 65535];
    // for ranges, the low end must not exceed the high end. No whitespace or
    // any other characters are permitted.
    bool isValidPortList(const std::wstring &ports);

    // Accepts the canonical "{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}" GUID form
    // (braces required; hex digits only). Delegates to IIDFromString so the parser
    // is strict — anything that wouldn't round-trip through StringFromGUID2/IIDFromString
    // is rejected.
    bool isValidGuid(const std::wstring &guid);
}
