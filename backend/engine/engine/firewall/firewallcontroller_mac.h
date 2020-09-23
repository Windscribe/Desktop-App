#ifndef FIREWALLCONTROLLER_MAC_H
#define FIREWALLCONTROLLER_MAC_H

#include "firewallcontroller.h"

//thread safe
class FirewallController_mac : public FirewallController
{
    Q_OBJECT
public:
    explicit FirewallController_mac(QObject *parent, IHelper *helper);
    ~FirewallController_mac() override;

    bool firewallOn(const QString &ip, bool bAllowLanTraffic) override;
    bool firewallChange(const QString &ip, bool bAllowLanTraffic) override;
    bool firewallOff() override;
    bool firewallActualState() override;

    bool whitelistPorts(const apiinfo::StaticIpPortsVector &ports) override;
    bool deleteWhitelistPorts() override;

    void setInterfaceToSkip_mac(const QString &interfaceToSkip) override;

private:
    QString interfaceToSkip_;
    QMutex mutex_;
    bool firewallOnImpl(const QString &ip, bool bAllowLanTraffic, const apiinfo::StaticIpPortsVector &ports);
};

#endif // FIREWALLCONTROLLER_MAC_H
