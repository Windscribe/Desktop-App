#pragma once

#include "firewallcontroller.h"
#include <QRecursiveMutex>
#include "engine/helper/helper.h"

//thread safe
class FirewallController_linux : public FirewallController
{
    Q_OBJECT
public:
    explicit FirewallController_linux(QObject *parent, Helper *helper);
    ~FirewallController_linux() override;

    void firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, bool isVpnConnected) override;
    void firewallOff() override;
    bool firewallActualState() override;

    bool whitelistPorts(const api_responses::StaticIpPortsVector &ports) override;
    bool deleteWhitelistPorts() override;

    void setVpnInterface_posix(const QString &vpnInterface) override;
    void setFirewallOnBoot(bool bEnable, const QSet<QString>& ipTable = QSet<QString>(), bool isAllowLanTraffic = false) override;

private:
    Helper *helper_;
    QString vpnInterface_;
    bool forceUpdateVpnInterface_;
    QRecursiveMutex mutex_;

    // Builds the validated firewall intent and hands it to the helper, which is the sole author of
    // the actual iptables rules. Rule construction and local-machine introspection that used to
    // live here now live in the privileged helper (src/helper/linux/firewallcontroller.cpp).
    bool firewallOnImpl(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig);
};
