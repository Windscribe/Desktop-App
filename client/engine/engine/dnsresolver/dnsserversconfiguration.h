#ifndef DNSSERVERSCONFIGURATION_H
#define DNSSERVERSCONFIGURATION_H

#include <QStringList>
#include <QMutex>
#include "engine/types/types.h"

// singleton, global current DNS-servers configuration
class DnsServersConfiguration
{
public:
    static DnsServersConfiguration &instance()
    {
        static DnsServersConfiguration s;
        return s;
    }

    // if ips is empty, then use default OS DNS
    // sets the DNS servers for all subsequent requests
    void setDnsServersPolicy(DNS_POLICY_TYPE policy);

    QStringList getCurrentDnsServers() const;

private:
    QStringList ips;
    mutable QMutex mutex_;

    QStringList dnsPolicyTypeToStringList(DNS_POLICY_TYPE dnsPolicyType);
};

#endif // DNSSERVERSCONFIGURATION_H
