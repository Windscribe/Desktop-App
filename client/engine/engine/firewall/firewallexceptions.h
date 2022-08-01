#ifndef FIREWALLEXCEPTIONS_H
#define FIREWALLEXCEPTIONS_H

#include <QSharedPointer>
#include "types/proxysettings.h"

class FirewallExceptions
{
public:
    FirewallExceptions();

    void setHostIPs(const QStringList &hostIPs);
    void setWhiteListedIPs(const QSet<QString> &ips);
    void setProxyIP(const types::ProxySettings &proxySettings);

    void setCustomRemoteIp(const QString &remoteIP, bool &bChanged);
    void setConnectingIp(const QString &connectingIp, bool &bChanged);
    void setDNSServerIp(const QString &dnsIp, bool &bChanged);


    void setDnsPolicy(DNS_POLICY_TYPE dnsPolicy);

    void setLocationsPingIps(const QStringList &listIps);
    void setCustomConfigPingIps(const QStringList &listIps);

    QString getIPAddressesForFirewall() const;
    QString getIPAddressesForFirewallForConnectedState(const QString &connectedIp) const;

private:
    QStringList hostIPs_;
    QSet<QString> whitelistedIPs_;
    QString proxyIP_;
    QString remoteIP_;
    QStringList locationsPingIPs_;
    QStringList customConfigsPingIPs_;
    QString connectingIp_;
    QString dnsIp_;
    DNS_POLICY_TYPE dnsPolicyType_;

};

#endif // FIREWALLEXCEPTIONS_H
