#pragma once

#include <QScopedPointer>

#include "engine/connectionmanager/connectors/iconnectionplatformpolicy.h"
#include "engine/dns/ctrldmanager/ictrldmanager.h"
#include "engine/dns/idnsconfigurator.h"
#include "engine/helper/helper.h"

class DnsConfigurator : public IDnsConfigurator
{
    Q_OBJECT

public:
    // Takes ownership of platformPolicy and ctrldManager; when null, the production implementations
    // are created (tests pass fakes).
    explicit DnsConfigurator(QObject *parent, Helper *helper, IConnectionPlatformPolicy *platformPolicy = nullptr,
                             ICtrldManager *ctrldManager = nullptr);

    void setConnectedDnsInfo(const types::ConnectedDnsInfo &info) override;

    bool prepare() override;
    const QStringList &dnsWhitelistIps() const override;
    QString primaryDnsServer() const override;
    QStringList tunnelDnsServers(const QString &defaultDns) const override;
    void overrideAdapterDns(AdapterGatewayInfo &adapterInfo) const override;
    void applyWhileConnected(const AdapterGatewayInfo &vpnAdapter, bool isWireGuard) override;
    void stopDnsProxy() override;
    void restoreSystemDns() override;

private:
    Helper *helper_;
    QScopedPointer<IConnectionPlatformPolicy> platformPolicy_;
    ICtrldManager *ctrldManager_;
    types::ConnectedDnsInfo connectedDnsInfo_;

    // DNS-leak-protection whitelist computed in prepare() (ctrld listen IP + plain-DNS upstreams).
    QStringList dnsWhitelistIps_;

    bool usesDnsProxy() const;
};
