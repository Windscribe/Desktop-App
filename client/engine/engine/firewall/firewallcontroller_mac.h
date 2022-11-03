#ifndef FIREWALLCONTROLLER_MAC_H
#define FIREWALLCONTROLLER_MAC_H

#include "firewallcontroller.h"
#include "engine/helper/helper_mac.h"
#include "engine/apiinfo/staticips.h"

#include <QTemporaryFile>

class Anchor;

//thread safe
class FirewallController_mac : public FirewallController
{
    Q_OBJECT
public:
    explicit FirewallController_mac(QObject *parent, IHelper *helper);

    bool firewallOn(const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig) override;
    bool firewallOff() override;
    bool firewallActualState() override;

    bool whitelistPorts(const apiinfo::StaticIpPortsVector &ports) override;
    bool deleteWhitelistPorts() override;

    void setInterfaceToSkip_posix(const QString &interfaceToSkip) override;
    void enableFirewallOnBoot(bool bEnable) override;

private:
    Helper_mac *helper_;
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

    bool isFirewallEnabled_;
    QSet<QString> windscribeIps_;
    QString interfaceToSkip_;
    bool isAllowLanTraffic_;
    bool isCustomConfig_;
    apiinfo::StaticIpPortsVector staticIpPorts_;

    QTemporaryFile tempFile_;

    void firewallOffImpl();
    QStringList lanTrafficRules() const;
    QStringList vpnTrafficRules(const QString &interfaceToSkip, bool bIsCustomConfig) const;
    void getFirewallStateFromPfctl(FirewallState &outState);
    bool checkInternalVsPfctlState();
    QString generatePfConfFile(const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, const QString &interfaceToSkip);
    bool generateTableFile(QTemporaryFile &tempFile, const QSet<QString> &ips);
    void updateVpnAnchor();
};

#endif // FIREWALLCONTROLLER_MAC_H
