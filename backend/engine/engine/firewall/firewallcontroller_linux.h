#ifndef FIREWALLCONTROLLER_LINUX_H
#define FIREWALLCONTROLLER_LINUX_H

#include "firewallcontroller.h"

//thread safe
class FirewallController_linux : public FirewallController
{
    Q_OBJECT
public:
    explicit FirewallController_linux(QObject *parent, IHelper *helper);
    ~FirewallController_linux() override;

    bool firewallOn(const QString &ip, bool bAllowLanTraffic) override;
    bool firewallChange(const QString &ip, bool bAllowLanTraffic) override;
    bool firewallOff() override;
    bool firewallActualState() override;

    bool whitelistPorts(const apiinfo::StaticIpPortsVector &ports) override;
    bool deleteWhitelistPorts() override;

    void setInterfaceToSkip_mac(const QString &interfaceToSkip) override;
};

#endif // FIREWALLCONTROLLER_LINUX_H
