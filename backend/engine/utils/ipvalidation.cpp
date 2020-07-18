#include "ipvalidation.h"

bool IpValidation::isIp(const QString &str)
{
    return ipRegex_.exactMatch(str);
}

bool IpValidation::isDomain(const QString &str)
{
    return domainRegex_.exactMatch(str);
}

bool IpValidation::isIpOrDomain(const QString &str)
{
    return (isIp(str) || isDomain(str));
}

// remove all symbols from "-" to "." (for example, ca-001.windscribe.com -> ca.windscribe.com
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
    ipRegex_.setPattern("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");

    domainRegex_.setPattern("^(([a-zA-Z]{1})|([a-zA-Z]{1}[a-zA-Z]{1})|([a-zA-Z]{1}[0-9]{1})|([0-9]{1}[a-zA-Z]{1})|([a-zA-Z0-9][a-zA-Z0-9-_]{1,61}[a-zA-Z0-9]))\\.([a-zA-Z]{2,6}|[a-zA-Z0-9-]{2,30}\\.[a-zA-Z]{2,3})$");
}
