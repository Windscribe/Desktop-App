#include "firewallcontroller_linux.h"
#include <QStandardPaths>
#include "utils/logger.h"
#include <QDir>
#include "engine/helper/ihelper.h"

FirewallController_linux::FirewallController_linux(QObject *parent, IHelper *helper) :
    FirewallController(parent)
{

}

FirewallController_linux::~FirewallController_linux()
{

}

bool FirewallController_linux::firewallOn(const QString &ip, bool bAllowLanTraffic)
{
    return true;
}

bool FirewallController_linux::firewallChange(const QString &ip, bool bAllowLanTraffic)
{
    return true;
}

bool FirewallController_linux::firewallOff()
{
    return true;
}

bool FirewallController_linux::firewallActualState()
{
    return true;
}

bool FirewallController_linux::whitelistPorts(const apiinfo::StaticIpPortsVector &ports)
{
    return true;
}

bool FirewallController_linux::deleteWhitelistPorts()
{
    return true;
}

void FirewallController_linux::setInterfaceToSkip_mac(const QString &interfaceToSkip)
{

}
