#pragma once

#include "firewallcontroller.h"
#include "engine/helper/helper.h"
#include "api_responses/staticips.h"

#include <QMutex>

//thread safe
class FirewallController_mac : public FirewallController
{
    Q_OBJECT
public:
    explicit FirewallController_mac(QObject *parent, Helper *helper);

    void firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, bool isVpnConnected) override;
    void firewallOff() override;
    bool firewallActualState() override;

    bool whitelistPorts(const api_responses::StaticIpPortsVector &ports) override;
    bool deleteWhitelistPorts() override;

    void setVpnInterface_posix(const QString &vpnInterface) override;
    void setFirewallOnBoot(bool bEnable, const QSet<QString>& ipTable = QSet<QString>(), bool isAllowLanTraffic = false) override;

private:
    Helper *helper_;
    QMutex mutex_;

    struct FirewallState
    {
        bool isEnabled = false;
        bool isBasicAppRulesCorrect = false;
        QSet<QString> allowedIps;
        QString vpnInterface;
        bool isAllowLanTraffic = false;
        bool isStaticIpPortsEmpty = true;
    };

    bool isAppFirewallEnabled_;
    QSet<QString> allowedIps_;
    QString vpnInterface_;
    bool isAllowLanTraffic_;
    bool isCustomConfig_;
    QString connectingIp_;
    api_responses::StaticIpPortsVector staticIpPorts_;

    void firewallOffImpl();
    void getFirewallStateFromPfctl(FirewallState &outState);
    bool checkInternalVsPfctlState(FirewallState *outFirewallState = nullptr);

    // Assembles the validated firewall intent from the cached state. The helper is the sole author
    // of the pf rules; rule construction and local-machine introspection that used to live here now
    // live in the privileged helper (src/helper/macos/firewallcontroller.cpp).
    FirewallConfig makeConfig() const;

    // Pushes an assembled config to the helper. On success marks the firewall enabled and returns
    // true; on failure the helper leaves the prior rules intact, so the caller keeps its cached
    // state unchanged and the next call retries.
    bool applyConfig(const FirewallConfig &config);

    // We have to save the state of pf (enabled/disabled) before enabling the firewall in order to restore it.
    // We should keep it in QSettings as we need to save it between program launches.
    void setPfWasEnabledState(bool b);
    bool isPfWasEnabled() const;
};
