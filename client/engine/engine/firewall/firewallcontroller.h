#pragma once

#include <QObject>
#include <QSet>
#include "api_responses/staticips.h"

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
    virtual void firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig);
    virtual void firewallOff();
    virtual bool firewallActualState() = 0;

    virtual bool whitelistPorts(const api_responses::StaticIpPortsVector &ports);
    virtual bool deleteWhitelistPorts();

    // Mac/Linux specific functions
    virtual void setInterfaceToSkip_posix(const QString &interfaceToSkip) = 0;
    virtual void setFirewallOnBoot(bool bEnable, const QSet<QString>& ipTable = QSet<QString>(), bool isAllowLanTraffic = false) = 0;

protected:
    bool isStateChanged();

    QString latestConnectingIp_;
    QSet<QString> latestIps_;
    bool latestAllowLanTraffic_;
    bool latestEnabledState_;
    bool latestIsCustomConfig_;
    api_responses::StaticIpPortsVector latestStaticIpPorts_;
    bool bInitialized_;
    bool bStateChanged_;
};
