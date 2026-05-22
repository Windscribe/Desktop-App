#include "uniqueiplist.h"
#include "utils/networkingvalidation.h"

void UniqueIpList::add(const QString &ip)
{
    if (NetworkingValidation::isIp(ip))
    {
        set_ << ip;
    }
}

QSet<QString> UniqueIpList::get() const
{
    return set_;
}
