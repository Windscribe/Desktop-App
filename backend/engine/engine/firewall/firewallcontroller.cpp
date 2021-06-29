#include "firewallcontroller.h"
#include "engine/helper/ihelper.h"

FirewallController::FirewallController(QObject *parent) : QObject(parent),
    latestAllowLanTraffic_(false), latestEnabledState_(false),
    bInitialized_(false), bStateChanged_(false)
{
}

bool FirewallController::firewallOn(const QString &ip, bool bAllowLanTraffic)
{
    if (!bInitialized_)
    {
        bStateChanged_ = true;
        bInitialized_ = true;
    }
    else
    {
        bStateChanged_ = (latestEnabledState_ != true || latestIp_ != ip || latestAllowLanTraffic_ != bAllowLanTraffic);
    }
    latestIp_ = ip;
    latestAllowLanTraffic_ = bAllowLanTraffic;
    latestEnabledState_ = true;
    return true;
}

bool FirewallController::firewallChange(const QString &ip, bool bAllowLanTraffic)
{
    // the same logic as in firewallOn
    return FirewallController::firewallOn(ip, bAllowLanTraffic);
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

bool FirewallController::whitelistPorts(const apiinfo::StaticIpPortsVector &ports)
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
    return whitelistPorts(apiinfo::StaticIpPortsVector());
}

bool FirewallController::isStateChanged()
{
    return bStateChanged_;
}

int FirewallController::countIps(const QString &ips)
{
    return ips.count(';') + 1;
}
