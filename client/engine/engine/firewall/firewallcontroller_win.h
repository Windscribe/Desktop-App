#ifndef FIREWALLCONTROLLER_WIN_H
#define FIREWALLCONTROLLER_WIN_H

#include "firewallcontroller.h"
#include "Engine/Helper/helper_win.h"

//thread safe
class FirewallController_win : public FirewallController
{
    Q_OBJECT
public:
    explicit FirewallController_win(QObject *parent, IHelper *helper);
    ~FirewallController_win() override;

    bool firewallOn(const QString &ip, bool bAllowLanTraffic) override;
    bool firewallOff() override;
    bool firewallActualState() override;

    bool whitelistPorts(const types::StaticIpPortsVector &ports) override;
    bool deleteWhitelistPorts() override;

    void setInterfaceToSkip_posix(const QString &interfaceToSkip) override;
    void enableFirewallOnBoot(bool bEnable) override;

private:
    Helper_win *helper_win_;
    QMutex mutex_;
};

#endif // FIREWALLCONTROLLER_WIN_H
