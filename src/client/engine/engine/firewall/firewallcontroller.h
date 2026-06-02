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
    virtual void firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, bool isVpnConnected);
    virtual void firewallOff();
    virtual bool firewallActualState() = 0;

    virtual bool whitelistPorts(const api_responses::StaticIpPortsVector &ports);
    virtual bool deleteWhitelistPorts();

    // Mac/Linux specific functions
    virtual void setVpnInterface_posix(const QString &vpnInterface) = 0;
    virtual void setFirewallOnBoot(bool bEnable, const QSet<QString>& ipTable = QSet<QString>(), bool isAllowLanTraffic = false) = 0;

protected:
    bool isStateChanged();

    // firewallOn()/firewallOff() only evaluate whether the desired state differs from the last
    // SUCCESSFULLY applied state; they no longer latch it. Platform subclasses call these commit
    // helpers after the apply actually succeeds, so a failed apply can't poison the change-detection
    // baseline — the next call still sees a change and retries instead of short-circuiting.
    void commitFirewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, bool isVpnConnected);
    void commitFirewallOff();

    QString latestConnectingIp_;
    QSet<QString> latestIps_;
    bool latestAllowLanTraffic_;
    bool latestIsCustomConfig_;
    bool latestEnabledState_;
    bool latestIsVpnConnected_;
    api_responses::StaticIpPortsVector latestStaticIpPorts_;
    bool bInitialized_;
    bool bStateChanged_;
};
