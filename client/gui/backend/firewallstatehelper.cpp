#include "firewallstatehelper.h"

FirewallStateHelper::FirewallStateHelper(QObject *parent) : QObject(parent),
    isEnabled_(false)
{

}

void FirewallStateHelper::firewallOnClickFromGUI()
{
    isEnabled_ = true;
    emit firewallStateChanged(true);
}

void FirewallStateHelper::firewallOffClickFromGUI()
{
    isEnabled_ = false;
    emit firewallStateChanged(false);
}

void FirewallStateHelper::setFirewallStateFromEngine(bool isEnabled)
{
    if (isEnabled_ != isEnabled)
    {
        isEnabled_ = isEnabled;
        emit firewallStateChanged(isEnabled_);
    }
}

bool FirewallStateHelper::isEnabled() const
{
    return isEnabled_;
}
