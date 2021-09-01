#include "firewallcontroller_win.h"
#include <QStandardPaths>
#include <QDir>
#include "Utils/logger.h"

FirewallController_win::FirewallController_win(QObject *parent, IHelper *helper) : FirewallController(parent)
{
    helper_win_ = dynamic_cast<Helper_win *>(helper);
    Q_ASSERT(helper_win_);
}

FirewallController_win::~FirewallController_win()
{

}

bool FirewallController_win::firewallOn(const QString &ip, bool bAllowLanTraffic)
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallOn(ip, bAllowLanTraffic);
    if (isStateChanged())
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "firewall enabled with ips count:" << countIps(ip);
        return helper_win_->firewallOn(ip, bAllowLanTraffic);
    }
    else
    {
        return true;
    }
}

bool FirewallController_win::firewallChange(const QString &ip, bool bAllowLanTraffic)
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallChange(ip, bAllowLanTraffic);
    if (isStateChanged())
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "firewall changed with ips count :" << countIps(ip);
        return helper_win_->firewallChange(ip, bAllowLanTraffic);
    }
    else
    {
        return true;
    }
}

bool FirewallController_win::firewallOff()
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallOff();
    if (isStateChanged())
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "firewall disabled";
        return helper_win_->firewallOff();
    }
    else
    {
        return true;
    }
}

bool FirewallController_win::firewallActualState()
{
    QMutexLocker locker(&mutex_);
    return helper_win_->firewallActualState();
}

bool FirewallController_win::whitelistPorts(const apiinfo::StaticIpPortsVector &ports)
{
    QMutexLocker locker(&mutex_);
    return helper_win_->whitelistPorts(ports.getAsStringWithDelimiters());
}

bool FirewallController_win::deleteWhitelistPorts()
{
    QMutexLocker locker(&mutex_);
    return helper_win_->deleteWhitelistPorts();
}

void FirewallController_win::setInterfaceToSkip_posix(const QString &interfaceToSkip)
{
    Q_UNUSED(interfaceToSkip);
    //nothing todo for Windows
}

void FirewallController_win::enableFirewallOnBoot(bool bEnable)
{
    Q_UNUSED(bEnable);
    //nothing todo for Windows

}
