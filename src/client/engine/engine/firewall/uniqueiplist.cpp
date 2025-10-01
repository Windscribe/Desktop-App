#include "uniqueiplist.h"
#include "utils/ipvalidation.h"

void UniqueIpList::add(const QString &ip)
{
    if (IpValidation::isIp(ip))
    {
        set_ << ip;
    }
}

QSet<QString> UniqueIpList::get() const
{
    return set_;
}
