#pragma once

#include <QString>

class IpValidation
{
public:
    static bool isIp(const QString &str);
    static bool isIpCidr(const QString &str);
    static bool isDomain(const QString &str);
    static bool isDomainWithWildcard(const QString &str);
    static bool isIpOrDomain(const QString &str);
    static bool isIpCidrOrDomain(const QString &str);
    static bool isCtrldCorrectAddress(const QString &str);

    static bool isValidIpForCidr(const QString &str);
    static bool isLocalIp(const QString &str);
    static bool isValidUrlForCtrld(const QString &str);
    static bool isWindscribeReservedIp(const QString &str);
    static QString getRemoteIdFromDomain(const QString &str);

#if defined(QT_DEBUG)
    static void runTests();
#endif
};
