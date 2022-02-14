#include "dnsserversconfiguration.h"
#include "utils/hardcodedsettings.h"
#include "utils/logger.h"
#include <QMutexLocker>

void DnsServersConfiguration::setDnsServersPolicy(DNS_POLICY_TYPE policy)
{
    QMutexLocker locker(&mutex_);
    ips = dnsPolicyTypeToStringList(policy);
    if (ips.isEmpty())
    {
        qCDebug(LOG_BASIC) << "Changed DNS servers for DnsResolver to OS default";
    }
    else
    {
        qCDebug(LOG_BASIC) << "Changed DNS servers for DnsResolver to:" << ips;
    }
}

QStringList DnsServersConfiguration::getCurrentDnsServers() const
{
    QMutexLocker locker(&mutex_);
    return ips;
}

QStringList DnsServersConfiguration::dnsPolicyTypeToStringList(DNS_POLICY_TYPE dnsPolicyType)
{
    if (dnsPolicyType == DNS_TYPE_OS_DEFAULT)
    {
        // return empty list for os default
    }
    else if (dnsPolicyType == DNS_TYPE_OPEN_DNS)
    {
        return HardcodedSettings::instance().openDns();
    }
    else if (dnsPolicyType == DNS_TYPE_CLOUDFLARE)
    {
        return QStringList() << HardcodedSettings::instance().cloudflareDns();
    }
    else if (dnsPolicyType == DNS_TYPE_GOOGLE)
    {
        return QStringList() << HardcodedSettings::instance().googleDns();
    }
    else if (dnsPolicyType == DNS_TYPE_CONTROLD)
    {
        return QStringList() << HardcodedSettings::instance().controldDns();
    }
    else
    {
        Q_ASSERT(false);
    }
    return QStringList();
}
