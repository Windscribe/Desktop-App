#ifndef FIREWALLCONTROLLER_MAC_H
#define FIREWALLCONTROLLER_MAC_H

#include "firewallcontroller.h"
#include "engine/helper/helper_mac.h"

//thread safe
class FirewallController_mac : public FirewallController
{
    Q_OBJECT
public:
    explicit FirewallController_mac(QObject *parent, IHelper *helper);
    ~FirewallController_mac() override;

    bool firewallOn(const QString &ip, bool bAllowLanTraffic) override;
    bool firewallOff() override;
    bool firewallActualState() override;

    bool whitelistPorts(const apiinfo::StaticIpPortsVector &ports) override;
    bool deleteWhitelistPorts() override;

    void setInterfaceToSkip_posix(const QString &interfaceToSkip) override;
    void enableFirewallOnBoot(bool bEnable) override;

private:
    Helper_mac *helper_;
    QString interfaceToSkip_;
    bool forceUpdateInterfaceToSkip_;
    QMutex mutex_;
    bool firewallOnImpl(const QString &ip, bool bAllowLanTraffic, const apiinfo::StaticIpPortsVector &ports);
};

#endif // FIREWALLCONTROLLER_MAC_H
