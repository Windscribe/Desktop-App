#ifndef FIREWALLCONTROLLER_MAC_H
#define FIREWALLCONTROLLER_MAC_H

#include "firewallcontroller.h"
#include "engine/helper/helper_mac.h"

#include <QTemporaryFile>

//thread safe
class FirewallController_mac : public FirewallController
{
    Q_OBJECT
public:
    explicit FirewallController_mac(QObject *parent, IHelper *helper);

    bool firewallOn(const QSet<QString> &ips, bool bAllowLanTraffic) override;
    bool firewallOff() override;
    bool firewallActualState() override;

    bool whitelistPorts(const types::StaticIpPortsVector &ports) override;
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
        bool isStaticIpPortsEmpty;
    };

    bool isFirewallEnabled_;
    QSet<QString> windscribeIps_;
    QString interfaceToSkip_;
    bool isAllowLanTraffic_;
    types::StaticIpPortsVector staticIpPorts_;

    QTemporaryFile tempFile_;

    void firewallOffImpl();
    QStringList lanTrafficRules() const;
    void getFirewallStateFromPfctl(FirewallState &outState);
    bool checkInternalVsPfctlState();
    QString generatePfConfFile(const QSet<QString> &ips, bool bAllowLanTraffic, const QString &interfaceToSkip);
    bool generateTableFile(QTemporaryFile &tempFile, const QSet<QString> &ips);
};

#endif // FIREWALLCONTROLLER_MAC_H
