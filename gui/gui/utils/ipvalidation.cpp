#include "ipvalidation.h"

bool IpValidation::isIp(const QString &str)
{
    return ipRegex_.exactMatch(str);
}

bool IpValidation::isIpCidr(const QString &str)
{
    return ipCidrRegex_.exactMatch(str);
}

bool IpValidation::isDomain(const QString &str)
{
    return domainRegex_.exactMatch(str);
}

bool IpValidation::isIpOrDomain(const QString &str)
{
    return (isIp(str) || isDomain(str));
}

bool IpValidation::isIpCidrOrDomain(const QString &str)
{
    return (isIpCidr(str) || isDomain(str));
}


IpValidation::IpValidation()
{
    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    ipRegex_.setPattern("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");
    ipCidrRegex_.setPattern("^([0-9]{1,3}\\.){3}[0-9]{1,3}(\\/([0-9]|[1-2][0-9]|3[0-2]))?$");
    domainRegex_.setPattern("^(([a-zA-Z]{1})|([a-zA-Z]{1}[a-zA-Z]{1})|([a-zA-Z]{1}[0-9]{1})|([0-9]{1}[a-zA-Z]{1})|([a-zA-Z0-9][a-zA-Z0-9-_]{1,61}[a-zA-Z0-9]))\\.([a-zA-Z]{2,6}|[a-zA-Z0-9-]{2,30}\\.[a-zA-Z]{2,3})$");
}
