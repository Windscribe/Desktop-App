#include "firewallcontroller_win.h"

#include <QStringList>

#include "utils/log/categories.h"

FirewallController_win::FirewallController_win(QObject *parent, Helper *helper) : FirewallController(parent),
    helper_(helper)
{
}

FirewallController_win::~FirewallController_win()
{
}

void FirewallController_win::firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, bool isVpnConnected)
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallOn(connectingIp, ips, bAllowLanTraffic, bIsCustomConfig, isVpnConnected);
    if (isStateChanged())
    {
        qCInfo(LOG_FIREWALL_CONTROLLER) << "firewall enabled with ips count:" << ips.count();
        helper_->firewallOn(connectingIp, QStringList(ips.begin(), ips.end()), bAllowLanTraffic, bIsCustomConfig);
    }
    commitFirewallOn(connectingIp, ips, bAllowLanTraffic, bIsCustomConfig, isVpnConnected);
}

void FirewallController_win::firewallOff()
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallOff();
    if (isStateChanged())
    {
        qCInfo(LOG_FIREWALL_CONTROLLER) << "firewall disabled";
        helper_->firewallOff();
    }
    commitFirewallOff();
}

bool FirewallController_win::firewallActualState()
{
    QMutexLocker locker(&mutex_);
    return helper_->firewallActualState();
}

bool FirewallController_win::whitelistPorts(const api_responses::StaticIpPortsVector &ports)
{
    QMutexLocker locker(&mutex_);
    return helper_->whitelistPorts(ports.getAsStringWithDelimiters());
}

bool FirewallController_win::deleteWhitelistPorts()
{
    QMutexLocker locker(&mutex_);
    return helper_->deleteWhitelistPorts();
}

void FirewallController_win::setVpnInterface_posix(const QString &vpnInterface)
{
    Q_UNUSED(vpnInterface);
    //nothing todo for Windows
}

void FirewallController_win::setFirewallOnBoot(bool bEnable, const QSet<QString> &ipTable, bool isAllowLanTraffic)
{
    // On Windows firewall on boot just retains existing firewall if enabled
    helper_->setFirewallOnBoot(bEnable);
}
