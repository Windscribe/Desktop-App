#include "firewallcontroller.h"

FirewallController::FirewallController(QObject *parent) : QObject(parent),
    latestAllowLanTraffic_(false), latestEnabledState_(false), latestIsVpnConnected_(false),
    bInitialized_(false), bStateChanged_(false)
{
}

void FirewallController::firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, bool isVpnConnected)
{
    if (!bInitialized_) {
        bStateChanged_ = true;
        bInitialized_ = true;
    } else {
        bStateChanged_ = (latestEnabledState_ != true ||
                          latestConnectingIp_ != connectingIp ||
                          latestIps_ != ips ||
                          latestAllowLanTraffic_ != bAllowLanTraffic ||
                          latestIsCustomConfig_ != bIsCustomConfig ||
                          latestIsVpnConnected_ != isVpnConnected);
    }
    latestConnectingIp_ = connectingIp;
    latestIps_ = ips;
    latestAllowLanTraffic_ = bAllowLanTraffic;
    latestIsCustomConfig_ = bIsCustomConfig;
    latestIsVpnConnected_ = isVpnConnected;
    latestEnabledState_ = true;
}

void FirewallController::firewallOff()
{
    if (!bInitialized_) {
        bStateChanged_ = true;
        bInitialized_ = true;
    } else {
        bStateChanged_ = (latestEnabledState_ != false);
    }
    latestEnabledState_ = false;
}

bool FirewallController::whitelistPorts(const api_responses::StaticIpPortsVector &ports)
{
    if (!bInitialized_) {
        bStateChanged_ = true;
        bInitialized_ = true;
    } else {
        bStateChanged_ = (latestStaticIpPorts_.getAsStringWithDelimiters() != ports.getAsStringWithDelimiters());
    }
    latestStaticIpPorts_ = ports;
    return true;
}

bool FirewallController::deleteWhitelistPorts()
{
    // the same logic as in whitelistPorts with empty static ip ports vector
    return whitelistPorts(api_responses::StaticIpPortsVector());
}

bool FirewallController::isStateChanged()
{
    return bStateChanged_;
}
