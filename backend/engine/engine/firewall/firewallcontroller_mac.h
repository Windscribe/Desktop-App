#ifndef FIREWALLCONTROLLER_MAC_H
#define FIREWALLCONTROLLER_MAC_H

#include "firewallcontroller.h"

//thread safe
class FirewallController_mac : public FirewallController
{
    Q_OBJECT
public:
    explicit FirewallController_mac(QObject *parent, IHelper *helper);
    virtual ~FirewallController_mac();

    virtual bool firewallOn(const QString &ip, bool bAllowLanTraffic);
    virtual bool firewallChange(const QString &ip, bool bAllowLanTraffic);
    virtual bool firewallOff();
    virtual bool firewallActualState();

    virtual bool whitelistPorts(const StaticIpPortsVector &ports);
    virtual bool deleteWhitelistPorts();

    virtual void setInterfaceToSkip_mac(const QString &interfaceToSkip);

private:
    QString interfaceToSkip_;
    QMutex mutex_;
    bool firewallOnImpl(const QString &ip, bool bAllowLanTraffic, const StaticIpPortsVector &ports);
};

#endif // FIREWALLCONTROLLER_MAC_H
