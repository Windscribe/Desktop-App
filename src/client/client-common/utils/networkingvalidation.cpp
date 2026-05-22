#include "networkingvalidation.h"

#include <cctype>
#include <QAbstractSocket>
#include <QHostAddress>
#include <QRegularExpression>

#include "ws_assert.h"

namespace {

// Strict parse of a bare IPv4 or IPv6 literal. Rejects CIDR suffix, scope id ("fe80::1%eth0"),
// and any whitespace — these forms are meaningless for the routes / firewall rules that
// NetworkingValidation feeds. Returns true on success; if `out` is non-null, the parsed address
// is written there.
bool parseIpLiteral(const QString &str, QHostAddress *out = nullptr)
{
    if (str.isEmpty()) {
        return false;
    }
    for (QChar c : str) {
        if (c == QLatin1Char('/') || c == QLatin1Char('%') ||
            c == QLatin1Char(' ') || c == QLatin1Char('\t')) {
            return false;
        }
    }
    QHostAddress addr;
    if (!addr.setAddress(str)) {
        return false;
    }
    // Defence-in-depth — QHostAddress::setAddress strips scope-id only when the literal contains
    // a '%', which we already reject above. The check is cheap and protects against future Qt
    // changes that might tolerate other scope-id encodings.
    if (!addr.scopeId().isEmpty()) {
        return false;
    }
    const auto proto = addr.protocol();
    if (proto != QAbstractSocket::IPv4Protocol && proto != QAbstractSocket::IPv6Protocol) {
        return false;
    }
    // QHostAddress::setAddress accepts BSD-style IPv4 shorthand ("1.2.3" → 1.2.0.3,
    // "1.2" → 1.0.0.2, "1" → 0.0.0.1) and leading-zero octets. Reject anything whose
    // input doesn't match Qt's canonical dotted-quad emission. Skipped for IPv6 because
    // legitimate IPv6 has multiple non-canonical-but-valid forms (uppercase hex, ::
    // compression variants) that callers may pass in.
    if (proto == QAbstractSocket::IPv4Protocol && addr.toString() != str) {
        return false;
    }
    if (out) {
        *out = addr;
    }
    return true;
}

// Validate "/N" suffix: digits only, in [0, maxPrefix]. Returns prefix or -1 on failure.
int parsePrefix(const QString &prefixStr, int maxPrefix)
{
    if (prefixStr.isEmpty()) {
        return -1;
    }
    for (QChar c : prefixStr) {
        if (c < QLatin1Char('0') || c > QLatin1Char('9')) {
            return -1;
        }
    }
    bool ok = false;
    const int value = prefixStr.toInt(&ok);
    if (!ok || value < 0 || value > maxPrefix) {
        return -1;
    }
    return value;
}

} // namespace


bool NetworkingValidation::isIp(const QString &str)
{
    return parseIpLiteral(str);
}

bool NetworkingValidation::isIpv4(const QString &str)
{
    QHostAddress addr;
    if (!parseIpLiteral(str, &addr)) {
        return false;
    }
    return addr.protocol() == QAbstractSocket::IPv4Protocol;
}

bool NetworkingValidation::isIpv6(const QString &str)
{
    QHostAddress addr;
    if (!parseIpLiteral(str, &addr)) {
        return false;
    }
    return addr.protocol() == QAbstractSocket::IPv6Protocol;
}

bool NetworkingValidation::isIpCidr(const QString &str)
{
    const int slashPos = str.indexOf(QLatin1Char('/'));
    const QString ipPart = (slashPos >= 0) ? str.left(slashPos) : str;
    QHostAddress addr;
    if (!parseIpLiteral(ipPart, &addr)) {
        return false;
    }
    if (slashPos < 0) {
        return true;
    }
    const int maxPrefix = (addr.protocol() == QAbstractSocket::IPv6Protocol) ? 128 : 32;
    return parsePrefix(str.mid(slashPos + 1), maxPrefix) >= 0;
}

bool NetworkingValidation::isDomain(const QString &str)
{
    if (str.size() > 253) {
        return false;
    }

    QRegularExpression domainRegex("^([a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?\\.){1,}([a-zA-Z][a-zA-Z0-9-]*[a-zA-Z])$");
    return domainRegex.match(str).hasMatch();
}

// the same as isDomain() but also allows the use the wildcard '*', for example "*.company.int" or "*.lol"
bool NetworkingValidation::isDomainWithWildcard(const QString &str)
{
    if (str.size() > 253) {
        return false;
    }

    QRegularExpression domainRegex("^([a-zA-Z0-9*]([a-zA-Z0-9-*]{0,61}[a-zA-Z0-9*])?\\.){1,}([a-zA-Z][a-zA-Z0-9-]*[a-zA-Z])$");
    return domainRegex.match(str).hasMatch();
}

bool NetworkingValidation::isIpOrDomain(const QString &str)
{
    return (isIp(str) || isDomain(str));
}

bool NetworkingValidation::isIpCidrOrDomain(const QString &str)
{
    return (isIpCidr(str) || isDomain(str));
}

// checking the correctness of the address for the ctrld utility
// If IP address -> legacy
// if https://..... -> DOH
// if hostname -> DOT
bool NetworkingValidation::isCtrldCorrectAddress(const QString &str)
{
    return isIp(str) || isDomain(str) || isValidUrlForCtrld(str);
}

bool NetworkingValidation::isValidIpForCidr(const QString &str)
{
    const int slashPos = str.indexOf(QLatin1Char('/'));
    if (slashPos < 0) {
        // No CIDR suffix → nothing to align. Preserve the historical "always true" semantics
        // even for non-IP inputs: callers (e.g. splittunnelingaddressesgroup) pass hostnames
        // through here after `isIpCidrOrDomain` succeeded, and the old implementation also
        // returned true for them via the `cidr_value == 32` early-out.
        return true;
    }
    QHostAddress addr;
    if (!parseIpLiteral(str.left(slashPos), &addr)) {
        return false;
    }

    const bool isV6 = (addr.protocol() == QAbstractSocket::IPv6Protocol);
    const int maxPrefix = isV6 ? 128 : 32;
    const int prefix = parsePrefix(str.mid(slashPos + 1), maxPrefix);
    if (prefix < 0) {
        return false;
    }

    if (!isV6) {
        // Keep the existing well-tested 32-bit arithmetic. /32 yields mask=0xFFFFFFFF so the
        // single-host case is trivially "aligned"; /0 yields mask=0 and requires ip==0.
        const quint32 ip_value = addr.toIPv4Address();
        const quint32 ip_mask = (prefix == 0) ? 0u
                                              : ((prefix >= 32) ? 0xFFFFFFFFu
                                                                : ~((1u << (32 - prefix)) - 1));
        return (ip_value & ip_mask) == ip_value;
    }

    // v6: walk 16 raw bytes, building a per-byte mask from the remaining bit count.
    const Q_IPV6ADDR raw = addr.toIPv6Address();
    int bitsRemaining = prefix;
    for (int i = 0; i < 16; ++i) {
        quint8 maskByte;
        if (bitsRemaining >= 8) {
            maskByte = 0xFF;
            bitsRemaining -= 8;
        } else if (bitsRemaining > 0) {
            maskByte = static_cast<quint8>(0xFFu << (8 - bitsRemaining));
            bitsRemaining = 0;
        } else {
            maskByte = 0;
        }
        const quint8 b = static_cast<quint8>(raw[i]);
        if ((b & maskByte) != b) {
            return false;
        }
    }
    return true;
}

bool NetworkingValidation::isLocalIp(const QString &str)
{
    // Tighten input first: require a valid IPv4 or IPv6 literal before classifying.
    // Practical impact is nil — callers (e.g. connecteddnsgroup) only pass user-typed full IPs —
    // but this avoids `startsWith` false positives like "10.foo" returning true.
    QHostAddress addr;
    if (!parseIpLiteral(str, &addr)) {
        return false;
    }

    if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
        // Preserve the existing startsWith checks verbatim — zero risk of behavior drift on the
        // four pre-existing ranges (https://en.wikipedia.org/wiki/Private_network).
        if (str.startsWith("127.") || str.startsWith("10.") || str.startsWith("192.168.") || str.startsWith("169.254.")) {
            return true;
        }
        else if (str.startsWith("172"))
        {
            const auto octets = str.split(".");
            if (octets.size() == 4) {
                const auto octet2 = octets.at(1).toInt();
                if (octet2 >= 16 && octet2 <= 31) {
                    return true;
                }
            }
        }
        return false;
    }

    // v6: ::1 (loopback), fe80::/10 (link-local), fc00::/7 (unique local unicast).
    return addr.isLoopback() || addr.isLinkLocal() || addr.isUniqueLocalUnicast();
}

bool NetworkingValidation::isValidUrlForCtrld(const QString &str)
{
    QRegularExpression httpsRegex("^((https|h3):\\/)\\/?([^:\\/\\s]+)((\\/\\w+)*\\/)([\\w\\-\\.]+[^#?\\s]+)(.*)?(#[\\w\\-]+)?$");
    QRegularExpression sdnsRegex("^sdns:\\/\\/[A-Za-z0-9]+$");
    return httpsRegex.match(str).hasMatch() || sdnsRegex.match(str).hasMatch();
}

bool NetworkingValidation::isReservedIp(const QString &str)
{
    return str.startsWith("10.255.255.");
}

bool NetworkingValidation::isValidMacAddress(const QString &str)
{
    const int len = str.size();
    if (len == 12) {
        for (QChar c : str) {
            if (!isxdigit(c.toLatin1())) {
                return false;
            }
        }
        return true;
    }
    if (len == 17) {
        const QChar separator = str.at(2);
        if (separator != QLatin1Char(':') && separator != QLatin1Char('-')) {
            return false;
        }
        for (int i = 0; i < len; ++i) {
            if (i % 3 == 2) {
                if (str.at(i) != separator) {
                    return false;
                }
            } else if (!isxdigit(str.at(i).toLatin1())) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool NetworkingValidation::isValidInterfaceName(const QString &str)
{
#if defined(Q_OS_WIN)
    // Windows interface names are adapter friendly names (e.g., "Ethernet 2", "Wi-Fi",
    // "Local Area Connection 7"). Bound by Win32 IF_MAX_STRING_SIZE (256). They are
    // user-renamable Unicode strings, so we only reject NUL and other control characters.
    constexpr int kMaxWinInterfaceLen = 256;
    if (str.isEmpty() || str.size() > kMaxWinInterfaceLen) {
        return false;
    }
    for (QChar c : str) {
        if (c == QChar(0) || c.category() == QChar::Other_Control) {
            return false;
        }
    }
    return true;
#else
    // IFNAMSIZ on Linux/macOS is 16 (including NUL terminator).
    constexpr int kMaxInterfaceLen = 16;
    if (str.isEmpty() || str.size() >= kMaxInterfaceLen) {
        return false;
    }
    for (QChar c : str) {
        if (c == QChar(0) || c == QLatin1Char(':') || c == QLatin1Char('/') || c.isSpace()) {
            return false;
        }
    }
    return true;
#endif
}

bool NetworkingValidation::isValidPort(int port)
{
    return port >= 1 && port <= 65535;
}

QString NetworkingValidation::getRemoteIdFromDomain(const QString &str)
{
    int ind1 = str.indexOf('-');
    if(ind1 == -1)
    {
        return str;
    }
    int ind2 = str.indexOf('.', ind1);
    if(ind2 == -1)
    {
        return str;
    }
    QString s = str;
    s = s.remove(ind1, ind2 - ind1);
    return s;
}

#if defined(QT_DEBUG)

void NetworkingValidation::runTests()
{
    // Valid domain names:
    const char *kValidDomainNames[] = {
        "g.co",
        "g.com",
        "google.t.t.co",
        "0-0o.com",
        "0-oz.co.uk",
        "0-google.com.br",
        "0-wh-ao14-0.com-com.net",
        "xn--d1ai6ai.xn--p1ai",
        "xn-fsqu00a.xn-0zwm56d",
        "xn--google.com",
        "google.xn--com",
        "google.com.au",
        "www.google.com",
        "google.com",
        "google123.com",
        "google-info.com",
        "sub.google.com",
        "sub.google-info.com",
        "my.sub-domain.at-123.google.com.au",
        "test-andmoretest-somerandomlettersoflongstring.us-east-2.eu.google.com",
        "a-1234567890-1234567890-1234567890-1234567890-1234567890-1234-z.eu.us",
    };

    // Invalid domain names:
    const char *kInvalidDomainNames[] = {
        "com.g",            // TLD must be 2 or more characters long.
        "google.t.t.c",     // TLD must be 2 or more characters long.
        "google,com",       // Comma is not allowed.
        "google",           // Missing TLD.
        "google.1test",     // TLD must not start with a digit.
        "google.test1",     // TLD must not end with a digit.
        "google.123",       // TLD must not start or end with a digit.
        ".com",             // Empty subdomain.
        "google.com/users", // No TLD.
        "-g.com",           // Subdomain cannot start with a hyphen.
        "-0-0o.com",        // Subdomain cannot start with a hyphen.
        "0-0o_.com",        // Underscore is not allowed.
        "-google.com",      // Subdomain cannot start with a hyphen.
        "google-.com",      // Subdomain cannot end with a hyphen.
        "sub.-google.com",  // Subdomain cannot start with a hyphen.
        "sub.google-.com",  // Subdomain cannot end with a hyphen.
        "a-1234567890-1234567890-1234567890-1234567890-1234567890-12345-z.eu.us", // Too long.
    };

    for (size_t i = 0; i < sizeof(kValidDomainNames) / sizeof(kValidDomainNames[0]); ++i)
        WS_ASSERT(isDomain(QString(kValidDomainNames[i])));
    for (size_t i = 0; i < sizeof(kInvalidDomainNames) / sizeof(kInvalidDomainNames[0]); ++i)
        WS_ASSERT(!isDomain(QString(kInvalidDomainNames[i])));

    // -------- isIp: valid v4 + v6 literals --------
    const char *kValidIps[] = {
        "0.0.0.0",
        "1.2.3.4",
        "255.255.255.255",
        "8.8.8.8",
        "::",
        "::1",
        "fe80::1",
        "2001:db8::1",
        "2001:4860:4860::8888",
        "fd00:abcd::5",
    };
    for (size_t i = 0; i < sizeof(kValidIps) / sizeof(kValidIps[0]); ++i)
        WS_ASSERT(isIp(QString(kValidIps[i])));

    // -------- isIp: invalid inputs --------
    const char *kInvalidIps[] = {
        "",
        "1.2.3",                // missing octet
        "1.2.3.4.5",            // too many octets
        "1.2.3.256",            // octet out of range
        "1.2.3.4/24",           // CIDR not allowed in isIp
        "google.com",           // hostname
        "fe80::1%eth0",         // scoped v6
        "::g",                  // bad hex in v6
        " 1.2.3.4",             // leading whitespace
        "1.2.3.4 ",             // trailing whitespace
        "2001:db8::/64",        // CIDR not allowed in isIp
    };
    for (size_t i = 0; i < sizeof(kInvalidIps) / sizeof(kInvalidIps[0]); ++i)
        WS_ASSERT(!isIp(QString(kInvalidIps[i])));

    // -------- isIpCidr: valid v4 + v6 --------
    const char *kValidIpCidrs[] = {
        "1.2.3.4",              // single host, no suffix
        "1.2.3.4/0",
        "1.2.3.4/24",
        "10.0.0.0/8",
        "0.0.0.0/0",
        "2001:db8::1",          // single host v6
        "2001:db8::/64",
        "::/0",
        "2001:db8::1/128",
        "fd00::/8",
    };
    for (size_t i = 0; i < sizeof(kValidIpCidrs) / sizeof(kValidIpCidrs[0]); ++i)
        WS_ASSERT(isIpCidr(QString(kValidIpCidrs[i])));

    // -------- isIpCidr: invalid --------
    const char *kInvalidIpCidrs[] = {
        "1.2.3.4/33",           // prefix > 32 for v4
        "2001:db8::/129",       // prefix > 128 for v6
        "garbage/24",           // not an IP
        "1.2.3.4 /24",          // whitespace
        "1.2.3.4/",             // empty prefix
        "1.2.3.4/-1",           // negative prefix
        "1.2.3.4/24/8",         // double suffix
        "2001:db8::1/abc",      // non-numeric prefix
    };
    for (size_t i = 0; i < sizeof(kInvalidIpCidrs) / sizeof(kInvalidIpCidrs[0]); ++i)
        WS_ASSERT(!isIpCidr(QString(kInvalidIpCidrs[i])));

    // -------- isValidIpForCidr: alignment checks --------
    WS_ASSERT(isValidIpForCidr("10.0.0.0/8"));
    WS_ASSERT(isValidIpForCidr("192.168.0.0/16"));
    WS_ASSERT(isValidIpForCidr("1.2.3.4"));               // single host, vacuously aligned
    WS_ASSERT(isValidIpForCidr("1.2.3.4/32"));
    WS_ASSERT(isValidIpForCidr("0.0.0.0/0"));
    WS_ASSERT(!isValidIpForCidr("10.0.0.1/8"));           // host bits set
    WS_ASSERT(!isValidIpForCidr("192.168.1.0/16"));       // host bits set
    WS_ASSERT(isValidIpForCidr("2001:db8::/64"));
    WS_ASSERT(isValidIpForCidr("2001:db8::1/128"));       // single host
    WS_ASSERT(isValidIpForCidr("::/0"));
    WS_ASSERT(!isValidIpForCidr("2001:db8::1/64"));       // host bits set in v6
    WS_ASSERT(!isValidIpForCidr("garbage/24"));           // CIDR-shaped but bad IP
    WS_ASSERT(isValidIpForCidr("google.com"));            // hostname-pass-through (preserved)

    // -------- isLocalIp --------
    WS_ASSERT(isLocalIp("127.0.0.1"));
    WS_ASSERT(isLocalIp("10.0.0.5"));
    WS_ASSERT(isLocalIp("192.168.1.1"));
    WS_ASSERT(isLocalIp("169.254.10.1"));
    WS_ASSERT(isLocalIp("172.16.0.1"));
    WS_ASSERT(isLocalIp("172.31.255.255"));
    WS_ASSERT(!isLocalIp("172.32.0.1"));                  // outside 172.16/12
    WS_ASSERT(!isLocalIp("172.15.0.1"));                  // outside 172.16/12
    WS_ASSERT(!isLocalIp("8.8.8.8"));
    WS_ASSERT(!isLocalIp("10.foo"));                      // tightening: not a valid IP
    WS_ASSERT(isLocalIp("::1"));
    WS_ASSERT(isLocalIp("fe80::1"));
    WS_ASSERT(isLocalIp("fc00::1"));
    WS_ASSERT(isLocalIp("fd12:3456::1"));
    WS_ASSERT(!isLocalIp("2001:4860:4860::8888"));        // public v6
}

#endif  // QT_DEBUG
