#include "dnsutils.h"
#include "engine/hardcodedsettings.h"

namespace DnsUtils
{

QStringList dnsPolicyTypeToStringList(DNS_POLICY_TYPE dnsPolicyType)
{
    if (dnsPolicyType == DNS_TYPE_OS_DEFAULT)
    {
        // return empty list for os default
    }
    else if (dnsPolicyType == DNS_TYPE_OPEN_DNS)
    {
        return HardcodedSettings::instance().customDns();
    }
    else if (dnsPolicyType == DNS_TYPE_CLOUDFLARE)
    {
        return QStringList() << HardcodedSettings::instance().cloudflareDns();
    }
    else if (dnsPolicyType == DNS_TYPE_GOOGLE)
    {
        return QStringList() << HardcodedSettings::instance().googleDns();
    }
    else
    {
        Q_ASSERT(false);
    }
    return QStringList();
}


}
