#ifndef FIREWALLEXCEPTIONS_H
#define FIREWALLEXCEPTIONS_H

#include <QSharedPointer>
#include "engine/proxy/proxysettings.h"

class FirewallExceptions
{
public:
    FirewallExceptions();

    void setHostIPs(const QStringList &hostIPs);
    void setProxyIP(const ProxySettings &proxySettings);

    void setCustomRemoteIp(const QString &remoteIP, bool &bChanged);
    void setCustomOvpnIp(const QString &ip, bool &bChanged);

    void setDnsPolicy(DNS_POLICY_TYPE dnsPolicy);

    void setLocationsIps(const QStringList &listIps);

    QString getIPAddressesForFirewall();
    QString getIPAddressesForFirewallForConnectedState(const QString &connectedIp);

private:
    QStringList hostIPs_;
    QString proxyIP_;
    QString remoteIP_;
    QStringList locationsIPs_;
    QString customOvpnIP_;
    DNS_POLICY_TYPE dnsPolicyType_;

};

#endif // FIREWALLEXCEPTIONS_H
