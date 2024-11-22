#include "ipvalidation.h"

#include <QRegExp>

#include "ws_assert.h"


bool IpValidation::isIpAddress(const QString &str)
{
    return isIpv4Address(str) || isIpv6Address(str);
}

bool IpValidation::isIpv4Address(const QString &str)
{
    QHostAddress address(str);
    return address.protocol() == QAbstractSocket::IPv4Protocol;
}

bool IpValidation::isIpv6Address(const QString &str)
{
    QHostAddress address(str);
    return address.protocol() == QAbstractSocket::IPv6Protocol;
}

bool IpValidation::isIpCidr(const QString &str)
{
    return isIpv4Cidr(str) || isIpv6Cidr(str);
}

bool IpValidation::isIpv4Cidr(const QString &str)
{
    QPair<QHostAddress, int> pair = QHostAddress::parseSubnet(str);
    return pair.first.protocol() == QAbstractSocket::IPv4Protocol && pair.second >= 0 && pair.second <= 32;
}

bool IpValidation::isIpv6Cidr(const QString &str)
{
    QPair<QHostAddress, int> pair = QHostAddress::parseSubnet(str);
    return pair.first.protocol() == QAbstractSocket::IPv6Protocol && pair.second >= 0 && pair.second <= 128;
}

bool IpValidation::isDomain(const QString &str)
{
    if (str.size() > 253) {
        return false;
    }

    QRegExp domainRegex("^([a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?\\.){1,}([a-zA-Z][a-zA-Z0-9-]*[a-zA-Z])$");
    return domainRegex.exactMatch(str);
}

// the same as isDomain() but also allows the use the wildcard '*', for example "*.company.int" or "*.lol"
bool IpValidation::isDomainWithWildcard(const QString &str)
{
    if (str.size() > 253) {
        return false;
    }

    QRegExp domainRegex("^([a-zA-Z0-9*]([a-zA-Z0-9-*]{0,61}[a-zA-Z0-9*])?\\.){1,}([a-zA-Z][a-zA-Z0-9-]*[a-zA-Z])$");
    return domainRegex.exactMatch(str);
}

bool IpValidation::isIpv4AddressOrDomain(const QString &str)
{
    return (isIpv4Address(str) || isDomain(str));
}

bool IpValidation::isIpCidrOrDomain(const QString &str)
{
    return (isIpv4Cidr(str) || isIpv6Cidr(str) || isDomain(str));
}

// checking the correctness of the address for the ctrld utility
// If IP address -> legacy
// if https://..... -> DOH
// if hostname -> DOT
bool IpValidation::isCtrldCorrectAddress(const QString &str)
{
    return isIpv4Address(str) || isDomain(str) || isValidUrlForCtrld(str);
}

bool IpValidation::isLocalIpv4Address(const QString &str)
{
    // NB: QHostAddress::isPrivateUse() is not available in Qt 6.5, so implement the logic manually

    // Rules are given from https://en.wikipedia.org/wiki/Private_network
    if(str.startsWith("127.") || str.startsWith("10.") || str.startsWith("192.168.") || str.startsWith("169.254.")) {
        return true;
    }
    else if(str.startsWith("172"))
    {
        const auto octets = str.split(".");
        if(octets.size() == 4) {
            const auto octet2 = octets.at(1).toInt();
            if(octet2 >= 16 && octet2 <= 31) {
                return true;
            }
        }
    }
    return false;
}

bool IpValidation::isValidUrlForCtrld(const QString &str)
{
    QRegExp httpsRegex("^((https|h3):\\/)\\/?([^:\\/\\s]+)((\\/\\w+)*\\/)([\\w\\-\\.]+[^#?\\s]+)(.*)?(#[\\w\\-]+)?$");
    QRegExp sdnsRegex("^sdns:\\/\\/[A-Za-z0-9]+$");
    return httpsRegex.exactMatch(str) || sdnsRegex.exactMatch(str);
}

bool IpValidation::isWindscribeReservedIp(const QString &str)
{
    return isIpv4Address(str) && str.startsWith("10.255.255.");
}

QString IpValidation::getRemoteIdFromDomain(const QString &str)
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

void IpValidation::runTests()
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
}

#endif  // QT_DEBUG
