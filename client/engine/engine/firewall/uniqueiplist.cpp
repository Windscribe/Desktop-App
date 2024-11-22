#include "uniqueiplist.h"
#include "utils/ipvalidation.h"

void UniqueIpList::add(const QString &ip)
{
    if (IpValidation::isIpv4Address(ip))
    {
        set_ << ip;
    }
}

QSet<QString> UniqueIpList::get() const
{
    return set_;
}
