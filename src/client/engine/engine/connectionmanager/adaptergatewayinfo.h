#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include "types/ipaddress.h"

class AdapterGatewayInfo
{
public:
    static AdapterGatewayInfo detectAndCreateDefaultAdapterInfo();

public:
    explicit AdapterGatewayInfo();
    void setAdapterName(const QString &name) { adapterName_ = name; }
    // Per-family interface name for the IPv6 default route. On a multi-homed host the
    // v6 default route can live on a different NIC than the v4 default; storing it
    // separately lets the helper emit v6 routes with the correct `dev`. Falls back to
    // the v4 adapter name when not set (single-homed and VPN-adapter cases).
    void setAdapterNameV6(const QString &name) { adapterNameV6_ = name; }

    void addAdapterIp(const types::IpAddress &ip);
    void addGatewayIp(const types::IpAddress &ip);
    void setDnsServers(const QVector<types::IpAddress> &ips) { dnsServers_ = ips; }
    // Mirrors addAdapterIp / addGatewayIp: silently drops invalid entries so callers
    // don't have to pre-filter.
    void addDnsServer(const types::IpAddress &ip);
    void setRemoteIp(const types::IpAddress &ip) { remoteIp_ = ip; }
    void setIfIndex(unsigned long ifIndex) { ifIndex_ = ifIndex; }
    QString adapterName() const { return adapterName_; }
    QString adapterNameV6() const { return adapterNameV6_.isEmpty() ? adapterName_ : adapterNameV6_; }

    types::IpAddress adapterIpV4() const;
    types::IpAddress adapterIpV6() const;
    types::IpAddress gatewayV4() const;
    types::IpAddress gatewayV6() const;

    QVector<types::IpAddress> dnsServers() const { return dnsServers_; }
    QStringList dnsServersAsStringList() const;
    types::IpAddress remoteIp() const { return remoteIp_; }
    unsigned long ifIndex() const { return ifIndex_; }

    void clear() {
        adapterName_.clear();
        adapterNameV6_.clear();
        adapterIps_.clear();
        gatewayIps_.clear();
        dnsServers_.clear();
        remoteIp_ = types::IpAddress();
    }

    bool isEmpty() const;

    QString makeLogString();

private:
    QString adapterName_;
    QString adapterNameV6_;
    QVector<types::IpAddress> adapterIps_;
    QVector<types::IpAddress> gatewayIps_;
    QVector<types::IpAddress> dnsServers_;

    // used only for openvpn connection
    types::IpAddress remoteIp_;

    unsigned long ifIndex_; // used only for Windows
};
