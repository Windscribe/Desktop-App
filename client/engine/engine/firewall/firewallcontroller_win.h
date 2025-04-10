#pragma once

#include <QMutex>
#include "firewallcontroller.h"
#include "engine/helper/helper.h"

//thread safe
class FirewallController_win : public FirewallController
{
    Q_OBJECT
public:
    explicit FirewallController_win(QObject *parent, Helper *helper);
    ~FirewallController_win() override;

    void firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig) override;
    void firewallOff() override;
    bool firewallActualState() override;

    bool whitelistPorts(const api_responses::StaticIpPortsVector &ports) override;
    bool deleteWhitelistPorts() override;

    void setInterfaceToSkip_posix(const QString &interfaceToSkip) override;
    void setFirewallOnBoot(bool bEnable, const QSet<QString>& ipTable = QSet<QString>(), bool isAllowLanTraffic = false) override;

private:
    Helper *helper_;
    QMutex mutex_;
};
