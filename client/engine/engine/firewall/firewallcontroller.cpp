#include "firewallcontroller.h"
#include "engine/helper/ihelper.h"

FirewallController::FirewallController(QObject *parent) : QObject(parent),
    latestAllowLanTraffic_(false), latestEnabledState_(false),
    bInitialized_(false), bStateChanged_(false)
{
}

bool FirewallController::firewallOn(const QSet<QString> &ips, bool bAllowLanTraffic)
{
    if (!bInitialized_)
    {
        bStateChanged_ = true;
        bInitialized_ = true;
    }
    else
    {
        bStateChanged_ = (latestEnabledState_ != true || latestIps_ != ips || latestAllowLanTraffic_ != bAllowLanTraffic);
    }
    latestIps_ = ips;
    latestAllowLanTraffic_ = bAllowLanTraffic;
    latestEnabledState_ = true;
    return true;
}

bool FirewallController::firewallOff()
{
    if (!bInitialized_)
    {
        bStateChanged_ = true;
        bInitialized_ = true;
    }
    else
    {
        bStateChanged_ = (latestEnabledState_ != false);
    }
    latestEnabledState_ = false;
    return true;
}

bool FirewallController::whitelistPorts(const types::StaticIpPortsVector &ports)
{
    if (!bInitialized_)
    {
        bStateChanged_ = true;
        bInitialized_ = true;
    }
    else
    {
        bStateChanged_ = (latestStaticIpPorts_.getAsStringWithDelimiters() != ports.getAsStringWithDelimiters());
    }
    latestStaticIpPorts_ = ports;
    return true;
}

bool FirewallController::deleteWhitelistPorts()
{
    // the same logic as in whitelistPorts with empty static ip ports vector
    return whitelistPorts(types::StaticIpPortsVector());
}

bool FirewallController::isStateChanged()
{
    return bStateChanged_;
}
