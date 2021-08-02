#include "firewallcontroller_linux.h"
#include <QStandardPaths>
#include "utils/logger.h"
#include "utils/utils.h"
#include <QDir>
#include "engine/helper/ihelper.h"

FirewallController_linux::FirewallController_linux(QObject *parent, IHelper *helper) :
    FirewallController(parent)
{
    Q_UNUSED(helper);
}

FirewallController_linux::~FirewallController_linux()
{
}

bool FirewallController_linux::firewallOn(const QString &ip, bool bAllowLanTraffic)
{
    Q_UNUSED(ip);
    Q_UNUSED(bAllowLanTraffic);
    return true;
}

bool FirewallController_linux::firewallChange(const QString &ip, bool bAllowLanTraffic)
{
    Q_UNUSED(ip);
    Q_UNUSED(bAllowLanTraffic);
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
    Q_UNUSED(ports);
    return true;
}

bool FirewallController_linux::deleteWhitelistPorts()
{
    return true;
}

void FirewallController_linux::setInterfaceToSkip_mac(const QString &interfaceToSkip)
{
    Q_UNUSED(interfaceToSkip);
}
