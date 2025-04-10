#include "firewallcontroller_win.h"
#include <QStandardPaths>
#include <QDir>
#include "utils/log/categories.h"

FirewallController_win::FirewallController_win(QObject *parent, Helper *helper) : FirewallController(parent),
    helper_(helper)
{
}

FirewallController_win::~FirewallController_win()
{
}

void FirewallController_win::firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig)
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallOn(connectingIp, ips, bAllowLanTraffic, bIsCustomConfig);
    if (isStateChanged())
    {
        QString ipStr;
        for (const QString &ip : ips)
        {
            ipStr += ip + ";";
        }

        if (ipStr.endsWith(";"))
        {
            ipStr = ipStr.remove(ipStr.length() - 1, 1);
        }

        qCInfo(LOG_FIREWALL_CONTROLLER) << "firewall enabled with ips count:" << ips.count();
        helper_->firewallOn(connectingIp, ipStr, bAllowLanTraffic, bIsCustomConfig);
    }
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

void FirewallController_win::setInterfaceToSkip_posix(const QString &interfaceToSkip)
{
    Q_UNUSED(interfaceToSkip);
    //nothing todo for Windows
}

void FirewallController_win::setFirewallOnBoot(bool bEnable, const QSet<QString> &ipTable, bool isAllowLanTraffic)
{
    // On Windows firewall on boot just retains existing firewall if enabled
    helper_->setFirewallOnBoot(bEnable);
}
