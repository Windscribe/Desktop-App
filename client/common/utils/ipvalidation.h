#ifndef IPVALIDATION_H
#define IPVALIDATION_H

#include <QString>
#include <QRegExp>

class IpValidation
{
public:
    static IpValidation &instance()
    {
        static IpValidation i;
        return i;
    }
    bool isIp(const QString &str);
    bool isIpCidr(const QString &str);
    bool isDomain(const QString &str);
    bool isIpOrDomain(const QString &str);
    bool isIpCidrOrDomain(const QString &str);
    bool isValidIpForCidr(const QString &str);
    bool isLocalIp(const QString &str);
    bool isWindscribeReservedIp(const QString &str);

    QString getRemoteIdFromDomain(const QString &str);

#if defined(QT_DEBUG)
    void runTests();
#endif

private:
    IpValidation();

    QRegExp ipRegex_;
    QRegExp ipCidrRegex_;
    QRegExp domainRegex_;
};

#endif // IPVALIDATION_H
