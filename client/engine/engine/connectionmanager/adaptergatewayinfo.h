#ifndef ADAPTER_GATEWAY_INFO_H
#define ADAPTER_GATEWAY_INFO_H

#include <QStringList>

class AdapterGatewayInfo
{
public:
    static AdapterGatewayInfo detectAndCreateDefaultAdapterInfo();

public:
    explicit AdapterGatewayInfo();
    void setAdapterName(const QString &name) { adapterName_ = name; }
    void setAdapterIp(const QString &ip) { adapterIp_ = ip; }
    void setGateway(const QString &ip) { gateway_ = ip; }
    void setDnsServers(const QStringList &ips) { dnsServers_ = ips; }
    void addDnsServer(const QString &ip) { dnsServers_ << ip; }
    void setRemoteIp(const QString &ip) { remoteIp_ = ip; }
    void setIfIndex(unsigned long ifIndex) { ifIndex_ = ifIndex; }

    QString adapterName() const { return adapterName_; }
    QString adapterIp() const { return adapterIp_; }
    QString gateway() const { return gateway_; }
    QStringList dnsServers() const { return dnsServers_; }
    QString remoteIp() const { return remoteIp_; }
    unsigned long ifIndex() const { return ifIndex_; }

    void clear() {
        adapterName_.clear();
        adapterIp_.clear();
        gateway_.clear();
        dnsServers_.clear();
        remoteIp_.clear();
    }

    bool isEmpty() const;

    QString makeLogString();

private:
    QString adapterName_;
    QString adapterIp_;
    QString gateway_;
    QStringList dnsServers_;

    // used only for openvpn connection
    QString remoteIp_;

    unsigned long ifIndex_; // used only for Windows
};

#endif // ADAPTER_GATEWAY_INFO_H
