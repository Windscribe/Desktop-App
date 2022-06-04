#ifndef FIREWALLCONTROLLER_LINUX_H
#define FIREWALLCONTROLLER_LINUX_H

#include "firewallcontroller.h"
#include "engine/helper/helper_linux.h"

//thread safe
class FirewallController_linux : public FirewallController
{
    Q_OBJECT
public:
    explicit FirewallController_linux(QObject *parent, IHelper *helper);
    ~FirewallController_linux() override;

    bool firewallOn(const QString &ip, bool bAllowLanTraffic) override;
    bool firewallOff() override;
    bool firewallActualState() override;

    bool whitelistPorts(const apiinfo::StaticIpPortsVector &ports) override;
    bool deleteWhitelistPorts() override;

    void setInterfaceToSkip_posix(const QString &interfaceToSkip) override;
    void enableFirewallOnBoot(bool bEnable) override;

private:
    Helper_linux *helper_;
    QString interfaceToSkip_;
    bool forceUpdateInterfaceToSkip_;
    QRecursiveMutex mutex_;
    QString pathToTempTable_;
    QString comment_;

    bool firewallOnImpl(const QString &ip, bool bAllowLanTraffic, const apiinfo::StaticIpPortsVector &ports);
    QStringList getWindscribeRules(const QString &comment, bool modifyForDelete, bool isIPv6);
    void removeWindscribeRules(const QString &comment, bool isIPv6);
};

#endif // FIREWALLCONTROLLER_LINUX_H
