#ifndef FIREWALLCONTROLLER_H
#define FIREWALLCONTROLLER_H

#include <QObject>
#include "engine/apiinfo/staticips.h"

class IHelper;

// basic class FirewallController
// implement state changes logic
class FirewallController : public QObject
{
    Q_OBJECT
public:
    explicit FirewallController(QObject *parent, IHelper *helper);
    virtual ~FirewallController() {}

    virtual bool firewallOn(const QString &ip, bool bAllowLanTraffic);
    virtual bool firewallChange(const QString &ip, bool bAllowLanTraffic);
    virtual bool firewallOff();
    virtual bool firewallActualState() = 0;

    virtual bool whitelistPorts(const ApiInfo::StaticIpPortsVector &ports);
    virtual bool deleteWhitelistPorts();

    // mac specific functions
    virtual void setInterfaceToSkip_mac(const QString &interfaceToSkip) = 0;

protected:
    bool isStateChanged();
    int countIps(const QString &ips);

    IHelper *helper_;
    QString latestIp_;
    bool latestAllowLanTraffic_;
    bool latestEnabledState_;
    ApiInfo::StaticIpPortsVector latestStaticIpPorts_;
    bool bInitialized_;
    bool bStateChanged_;
};

#endif // FIREWALLCONTROLLER_H
