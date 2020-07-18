#include "uniqueiplist.h"
#include "utils/ipvalidation.h"

void UniqueIpList::add(const QString &ip)
{
    if (IpValidation::instance().isIp(ip))
    {
        set_ << ip;
    }
}

QString UniqueIpList::getFirewallString()
{
    QString ret;
    Q_FOREACH(const QString &ip, set_)
    {
        ret += ip + ";";
    }

    if (ret.endsWith(";"))
    {
        ret = ret.remove(ret.length() - 1, 1);
    }

    return ret;
}
