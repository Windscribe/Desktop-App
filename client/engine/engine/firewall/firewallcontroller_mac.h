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

    bool firewallOn(const QSet<QString> &ips, bool bAllowLanTraffic) override;
    bool firewallOff() override;
    bool firewallActualState() override;

    bool whitelistPorts(const types::StaticIpPortsVector &ports) override;
    bool deleteWhitelistPorts() override;

    void setInterfaceToSkip_posix(const QString &interfaceToSkip) override;
    void enableFirewallOnBoot(bool bEnable) override;

private:
    Helper_mac *helper_;
    bool forceUpdateInterfaceToSkip_;
    QMutex mutex_;
    bool firewallOnImpl(const QString &ip, bool bAllowLanTraffic, const types::StaticIpPortsVector &ports);
    void firewallOffImpl();
    void updateIPs();

    struct FirewallState
    {
        bool isEnabled;
        bool isBasicWindscribeRulesCorrect;
        QSet<QString> windscribeIps;
        QString interfaceToSkip;
        bool isAllowLanTraffic;
    };

    bool isFirewallEnabled_;
    QSet<QString> windscribeIps_;
    QString interfaceToSkip_;
    bool isAllowLanTraffic_;

    QStringList lanTrafficRules() const;
    void getFirewallStateFromPfctl(FirewallState &outState);
    void checkInternalVsPfctlState();
    QString generatePfConfFile(const QSet<QString> &ips, bool bAllowLanTraffic, const QString &interfaceToSkip);
    QString generateTableFile(const QSet<QString> &ips);
    QString generateInterfaceToSkipAnchorFile(const QString &interfaceToSkip);
    QString generateLanTrafficAnchorFile(bool bAllowLanTraffic);
};

#endif // FIREWALLCONTROLLER_MAC_H
