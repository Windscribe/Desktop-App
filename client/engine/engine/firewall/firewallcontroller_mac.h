#pragma once

#include "firewallcontroller.h"
#include "engine/helper/helper.h"
#include "api_responses/staticips.h"

#include <QTemporaryFile>
#include <QMutex>

class Anchor;

//thread safe
class FirewallController_mac : public FirewallController
{
    Q_OBJECT
public:
    explicit FirewallController_mac(QObject *parent, Helper *helper);

    void firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, bool isVpnConnected) override;
    void firewallOff() override;
    bool firewallActualState() override;

    bool whitelistPorts(const api_responses::StaticIpPortsVector &ports) override;
    bool deleteWhitelistPorts() override;

    void setInterfaceToSkip_posix(const QString &interfaceToSkip) override;
    void setFirewallOnBoot(bool bEnable, const QSet<QString>& ipTable = QSet<QString>(), bool isAllowLanTraffic = false) override;

private:
    Helper *helper_;
    QMutex mutex_;

    struct FirewallState
    {
        bool isEnabled;
        bool isBasicWindscribeRulesCorrect;
        QSet<QString> windscribeIps;
        QString interfaceToSkip;
        bool isAllowLanTraffic;
        bool isCustomConfig;
        bool isStaticIpPortsEmpty;
    };

    QStringList awdl_p2p_interfaces_;

    bool isWindscribeFirewallEnabled_;
    QSet<QString> windscribeIps_;
    QString interfaceToSkip_;
    bool isAllowLanTraffic_;
    bool isCustomConfig_;
    QString connectingIp_;
    api_responses::StaticIpPortsVector staticIpPorts_;

    QTemporaryFile tempFile_;

    void firewallOffImpl();
    QStringList lanTrafficRules(bool bAllowLanTraffic) const;
    QStringList vpnTrafficRules(const QString &connectingIp, const QString &interfaceToSkip, bool bIsCustomConfig) const;
    void getFirewallStateFromPfctl(FirewallState &outState);
    bool checkInternalVsPfctlState(FirewallState *outFirewallState = nullptr);
    QString generatePfConf(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, const QString &interfaceToSkip);
    QString generateTable(const QSet<QString> &ips);
    void updateVpnAnchor();
    QStringList getLocalAddresses(const QString iface) const;

    // We have to save the state of pf (enabled/disabled) before enabling the firewall in order to restore it.
    // We should keep it in QSettings as we need to save it between program launches.
    void setPfWasEnabledState(bool b);
    bool isPfWasEnabled() const;
};
