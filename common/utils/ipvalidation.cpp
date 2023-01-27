#include "ipvalidation.h"
#include <QStringList>

bool IpValidation::isIp(const QString &str)
{
    return ipRegex_.match(str).hasMatch();
}

bool IpValidation::isIpCidr(const QString &str)
{
    return ipCidrRegex_.match(str).hasMatch();
}

bool IpValidation::isDomain(const QString &str)
{
    return str.size() <= 253 && domainRegex_.match(str).hasMatch();
}

bool IpValidation::isIpOrDomain(const QString &str)
{
    return (isIp(str) || isDomain(str));
}

bool IpValidation::isIpCidrOrDomain(const QString &str)
{
    return (isIpCidr(str) || isDomain(str));
}

bool IpValidation::isValidIpForCidr(const QString &str)
{
    const auto ip_and_cidr = str.split("/", Qt::SkipEmptyParts);
    const quint32 cidr_value = (ip_and_cidr.size() < 2) ? 32 : ip_and_cidr[1].toUInt();
    if (cidr_value == 32) {
        // CIDR is 32 or not specified, this is a single IP.
        return true;
    }
    const auto octets = ip_and_cidr[0].split(".");
    const quint32 ip_value = (octets[0].toUInt() << 24) | (octets[1].toUInt() << 16)
                            | (octets[2].toUInt() << 8) | octets[3].toUInt();
    const quint32 ip_mask = cidr_value ? ~((1 << (32 - cidr_value)) - 1) : 0;
    return (ip_value & ip_mask) == ip_value;
}

bool IpValidation::isLocalIp(const QString &str)
{
    // Rules are given from https://en.wikipedia.org/wiki/Private_network
    if(str.startsWith("127.") || str.startsWith("10.") || str.startsWith("192.168.")) {
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

IpValidation::IpValidation()
{
    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    // note: the patterns are "anchored" (^...$)
    // so no need for QRegExp::exactMatch (qt5) or QRegularExpression::anchoredPattern (qt6)
    ipRegex_.setPattern("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");
    ipCidrRegex_.setPattern("^([0-9]{1,3}\\.){3}[0-9]{1,3}(\\/([0-9]|[1-2][0-9]|3[0-2]))?$");
    domainRegex_.setPattern("^([a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?\\.){1,}([a-zA-Z][a-zA-Z0-9-]*[a-zA-Z])$");
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
        Q_ASSERT(IpValidation::instance().isDomain(QString(kValidDomainNames[i])));
    for (size_t i = 0; i < sizeof(kInvalidDomainNames) / sizeof(kInvalidDomainNames[0]); ++i)
        Q_ASSERT(!IpValidation::instance().isDomain(QString(kInvalidDomainNames[i])));
}

#endif  // QT_DEBUG
