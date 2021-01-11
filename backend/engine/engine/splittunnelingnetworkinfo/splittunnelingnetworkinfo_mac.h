#ifndef SPLITTUNNELINGNETWORKINFO_MAC_H
#define SPLITTUNNELINGNETWORKINFO_MAC_H

#include <QStringList>
#include "engine/types/protocoltype.h"

// Keep network info which need send to helper for split tunneling (currently used for Mac only)
class SplitTunnelingNetworkInfo
{
public:
    SplitTunnelingNetworkInfo();

    void setProtocol(const ProtocolType &protocol);

    // need call before connection
    void detectDefaultRoute();

    // need call after connection established
    void detectInfoFromDnsScript();
    void setVpnAdapterName(const QString &vpnName);
    void setIkev2DnsServers(const QStringList &dnsList);
    void setConnectedIp(const QString &ip);

    // getters
    ProtocolType protocol() const;

    QString gatewayIp() const;
    QString interfaceName() const;
    QString interfaceIp() const;

    QString connectedIp() const;

    QString routeVpnGateway() const;
    QString routeNetGateway() const;
    QString remote1() const;
    QString ifconfigTunIp() const;

    QString vpnAdapterName() const;
    QString vpnAdapterIpAddress() const;

    QStringList dnsServers() const;

    void outToLog();

private:
    ProtocolType protocol_;

    QString gatewayIp_;
    QString interfaceName_;
    QString interfaceIp_;

    QString connectedIp_;

    QString routeVpnGateway_;
    QString routeNetGateway_;
    QString remote1_;
    QString ifconfigTunIp_;

    QString vpnAdapterName_;
    QString vpnAdapterIpAddress_;

    QStringList dnsServers_;

    // parse line (fieldName=value)
    bool parseLine(const QString &line, QString &fieldName, QString &value);
};

#endif // SPLITTUNNELINGNETWORKINFO_MAC_H
