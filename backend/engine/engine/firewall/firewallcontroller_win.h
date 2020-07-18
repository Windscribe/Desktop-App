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
    virtual ~FirewallController_win();

    virtual bool firewallOn(const QString &ip, bool bAllowLanTraffic);
    virtual bool firewallChange(const QString &ip, bool bAllowLanTraffic);
    virtual bool firewallOff();
    virtual bool firewallActualState();

    virtual bool whitelistPorts(const StaticIpPortsVector &ports);
    virtual bool deleteWhitelistPorts();

    virtual void setInterfaceToSkip_mac(const QString &interfaceToSkip);

private:
    Helper_win *helper_win_;
    QMutex mutex_;
};

#endif // FIREWALLCONTROLLER_WIN_H
