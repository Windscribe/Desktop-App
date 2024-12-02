#include "dnsserversconfiguration.h"
#include "utils/ws_assert.h"
#include "utils/hardcodedsettings.h"
#include "utils/log/logger.h"
#include <QMutexLocker>

void DnsServersConfiguration::setDnsServersPolicy(DNS_POLICY_TYPE policy)
{
    QMutexLocker locker(&mutex_);
    ips = dnsPolicyTypeToStringList(policy);
    if (ips.isEmpty())
    {
        qCInfo(LOG_BASIC) << "Changed DNS servers for DnsResolver to OS default";
    }
    else
    {
        qCInfo(LOG_BASIC) << "Changed DNS servers for DnsResolver to:" << ips;
    }
}

std::vector<std::string> DnsServersConfiguration::getCurrentDnsServers() const
{
    QMutexLocker locker(&mutex_);
    if (isConnectedToVpn_) {
    // For windows c-ares library puts the loopback address in low priority and this
    // creates unnecessary delay when executing a dns request
    // As a workaround, we form a list of servers ourselves for a loopback adress, bypassing c-ares.
#ifdef Q_OS_WIN
        if (vpnDnsServers_.size() == 1 && vpnDnsServers_[0].startsWith("127.0.0")) {
            return toStdVector(vpnDnsServers_);
        }
#endif
        return std::vector<std::string>();       // use OS default DNS
    } else {
        return toStdVector(ips);
    }
}

void DnsServersConfiguration::setConnectedState(const QStringList &vpnDnsServers)
{
    QMutexLocker locker(&mutex_);
    vpnDnsServers_ = vpnDnsServers;
    isConnectedToVpn_ = true;
}

void DnsServersConfiguration::setDisconnectedState()
{
    QMutexLocker locker(&mutex_);
    vpnDnsServers_.clear();
    isConnectedToVpn_ = false;
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
        WS_ASSERT(false);
    }
    return QStringList();
}

std::vector<std::string> DnsServersConfiguration::toStdVector(const QStringList &list) const
{
    std::vector<std::string> v;
    for (auto &it : list)
        v.push_back(it.toStdString());
    return v;
}
