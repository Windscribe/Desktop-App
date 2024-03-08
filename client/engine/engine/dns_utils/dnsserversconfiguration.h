#pragma once

#include <QStringList>
#include <QMutex>
#include "types/enums.h"

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

    std::vector<std::string> getCurrentDnsServers() const;

    void setConnectedState(const QStringList &vpnDnsServers);
    void setDisconnectedState();

private:
    QStringList ips;

    QStringList vpnDnsServers_;
    bool isConnectedToVpn_ = false;
    mutable QMutex mutex_;

    QStringList dnsPolicyTypeToStringList(DNS_POLICY_TYPE dnsPolicyType);
    std::vector<std::string> toStdVector(const QStringList &list) const;
};
