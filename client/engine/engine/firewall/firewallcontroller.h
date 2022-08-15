#ifndef FIREWALLCONTROLLER_H
#define FIREWALLCONTROLLER_H

#include <QObject>
#include <QSet>
#include "types/staticips.h"

class IHelper;

// basic class FirewallController
// implement state changes logic
class FirewallController : public QObject
{
    Q_OBJECT
public:
    explicit FirewallController(QObject *parent);
    virtual ~FirewallController() {}

    // this function also uses for change firewall ips, then it is already enabled
    virtual bool firewallOn(const QSet<QString> &ips, bool bAllowLanTraffic);
    virtual bool firewallOff();
    virtual bool firewallActualState() = 0;

    virtual bool whitelistPorts(const types::StaticIpPortsVector &ports);
    virtual bool deleteWhitelistPorts();

    // Mac/Linux specific functions
    virtual void setInterfaceToSkip_posix(const QString &interfaceToSkip) = 0;
    virtual void enableFirewallOnBoot(bool bEnable) = 0;

protected:
    bool isStateChanged();

    QSet<QString> latestIps_;
    bool latestAllowLanTraffic_;
    bool latestEnabledState_;
    types::StaticIpPortsVector latestStaticIpPorts_;
    bool bInitialized_;
    bool bStateChanged_;
};

#endif // FIREWALLCONTROLLER_H
