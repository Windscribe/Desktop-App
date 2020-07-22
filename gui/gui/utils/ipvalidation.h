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

private:
    IpValidation();

    QRegExp ipRegex_;
    QRegExp ipCidrRegex_;
    QRegExp domainRegex_;
};

#endif // IPVALIDATION_H
