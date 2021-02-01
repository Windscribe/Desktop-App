#ifndef CONNECTION_ADAPTER_INFO_H
#define CONNECTION_ADAPTER_INFO_H

#include <QStringList>

class ConnectionAdapterInfo
{
public:
    static const QString wintunAdapterName;     // wintun adapter name
    static const QString tapAdapterName;        // old tap adapter name


public:
    void setAdapterName(const QString &name) { adapterName_ = name; }
    void setAdapterIp(const QString &ip) { adapterIp_ = ip; }
    void setVpnGateway(const QString &ip) { vpnGateway_ = ip; }
    void setDnsServers(const QStringList &ips) { dnsServers_ = ips; }
    void addDnsServer(const QString &ip) { dnsServers_ << ip; }

    QString adapterName() const { return adapterName_; }
    QString adapterIp() const { return adapterIp_; }
    QString vpnGateway() const { return vpnGateway_; }
    QStringList dnsServers() const { return dnsServers_; }

    void clear() {
        adapterName_.clear();
        adapterIp_.clear();
        vpnGateway_.clear();
        dnsServers_.clear();
    }

    void outToLog();

private:
    QString adapterName_;
    QString adapterIp_;
    QString vpnGateway_;
    QStringList dnsServers_;
};

#endif // CONNECTION_ADAPTER_INFO_H
