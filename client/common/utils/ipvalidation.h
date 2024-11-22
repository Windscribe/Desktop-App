#pragma once

#include <QHostAddress>
#include <QString>

class IpValidation
{
public:
    static bool isIpAddress(const QString &str);
    static bool isIpv4Address(const QString &str);
    static bool isIpv6Address(const QString &str);
    static bool isIpCidr(const QString &str);
    static bool isIpv4Cidr(const QString &str);
    static bool isIpv6Cidr(const QString &str);
    static bool isDomain(const QString &str);
    static bool isDomainWithWildcard(const QString &str);
    static bool isIpv4AddressOrDomain(const QString &str);
    static bool isIpCidrOrDomain(const QString &str);
    static bool isCtrldCorrectAddress(const QString &str);

    static bool isLocalIpv4Address(const QString &str);
    static bool isValidUrlForCtrld(const QString &str);
    static bool isWindscribeReservedIp(const QString &str);
    static QString getRemoteIdFromDomain(const QString &str);

#if defined(QT_DEBUG)
    static void runTests();
#endif
};
