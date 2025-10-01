#pragma once

#include <QSharedPointer>
#include "types/proxysettings.h"

class FirewallExceptions
{
public:
    FirewallExceptions();

    void setHostIPs(const QSet<QString> &hostIPs);
    void setProxyIP(const types::ProxySettings &proxySettings);

    void setCustomRemoteIp(const QString &remoteIP, bool &bChanged);
    void setConnectingIp(const QString &connectingIp, bool &bChanged);
    void setDNSServers(const QStringList &ips, bool &bChanged);

    void setDnsPolicy(DNS_POLICY_TYPE dnsPolicy);

    void setLocationsPingIps(const QStringList &listIps);
    void setCustomConfigPingIps(const QStringList &listIps);

    QSet<QString> getIPAddressesForFirewall() const;
    QSet<QString> getIPAddressesForFirewallForConnectedState() const;

    const QString& connectingIp() const;

private:
    QSet<QString> hostIPs_;
    QString proxyIP_;
    QString remoteIP_;
    QStringList locationsPingIPs_;
    QStringList customConfigsPingIPs_;
    QString connectingIp_;
    QStringList dnsIps_;
    DNS_POLICY_TYPE dnsPolicyType_;

};
