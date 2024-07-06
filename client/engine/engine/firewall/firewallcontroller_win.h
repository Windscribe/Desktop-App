#pragma once

#include "firewallcontroller.h"
#include "Engine/Helper/helper_win.h"

//thread safe
class FirewallController_win : public FirewallController
{
    Q_OBJECT
public:
    explicit FirewallController_win(QObject *parent, IHelper *helper);
    ~FirewallController_win() override;

    bool firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig) override;
    bool firewallOff() override;
    bool firewallActualState() override;

    bool whitelistPorts(const api_responses::StaticIpPortsVector &ports) override;
    bool deleteWhitelistPorts() override;

    void setInterfaceToSkip_posix(const QString &interfaceToSkip) override;
    void setFirewallOnBoot(bool bEnable, const QSet<QString>& ipTable = QSet<QString>(), bool isAllowLanTraffic = false) override;

private:
    Helper_win *helper_win_;
    QMutex mutex_;
};
