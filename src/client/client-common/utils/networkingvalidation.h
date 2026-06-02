#pragma once

#include <QString>

class NetworkingValidation
{
public:
    static bool isIp(const QString &str);
    // Strict v4/v6 literal classifiers. Both return false on hostnames, CIDR-suffixed input,
    // scope-id forms, or anything else `isIp()` would also reject.
    static bool isIpv4(const QString &str);
    static bool isIpv6(const QString &str);
    static bool isIpCidr(const QString &str);
    static bool isDomain(const QString &str);
    static bool isDomainWithWildcard(const QString &str);
    static bool isIpOrDomain(const QString &str);
    static bool isIpCidrOrDomain(const QString &str);
    static bool isCtrldCorrectAddress(const QString &str);

    static bool isValidIpForCidr(const QString &str);
    static bool isLocalIp(const QString &str);
    // True for the IPv4/IPv6 "unspecified" (any/wildcard) literals 0.0.0.0 and ::.
    static bool isUnspecifiedIp(const QString &str);
    static bool isValidUrlForCtrld(const QString &str);
    static bool isReservedIp(const QString &str);
    static QString getRemoteIdFromDomain(const QString &str);

    // Accepts XX:XX:XX:XX:XX:XX, XX-XX-XX-XX-XX-XX, or 12 contiguous hex digits.
    static bool isValidMacAddress(const QString &str);
    // Non-empty length-bounded interface name. POSIX (Linux/macOS): IFNAMSIZ-style — < 16 chars,
    // no NUL / ':' / '/' / whitespace. Windows: bounded by IF_MAX_STRING_SIZE (256), reject NUL
    // and other control characters; spaces and punctuation are allowed (friendly names like
    // "Ethernet 2", "Local Area Connection 7").
    static bool isValidInterfaceName(const QString &str);
    // 1..65535
    static bool isValidPort(int port);

    // Returns true iff every character is printable ASCII (0x20..0x7E). Rejects newlines,
    // tabs, NULs, and all non-ASCII. Empty string is considered valid; callers that need to
    // also reject empty values should check that separately. Used at boundaries that hand a
    // value off to a line-oriented sink (wireguard.conf, AmneziaWG UAPI, INI files) where a
    // smuggled newline could be reinterpreted as a new record/directive.
    static bool isPrintableSingleLineAscii(const QString &str);
};
