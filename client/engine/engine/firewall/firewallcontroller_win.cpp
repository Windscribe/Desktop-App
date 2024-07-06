#include "firewallcontroller_win.h"
#include <QStandardPaths>
#include <QDir>
#include "utils/ws_assert.h"
#include "utils/logger.h"

FirewallController_win::FirewallController_win(QObject *parent, IHelper *helper) : FirewallController(parent)
{
    helper_win_ = dynamic_cast<Helper_win *>(helper);
    WS_ASSERT(helper_win_);
}

FirewallController_win::~FirewallController_win()
{

}

bool FirewallController_win::firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig)
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

        qCDebug(LOG_FIREWALL_CONTROLLER) << "firewall enabled with ips count:" << ips.count();
        return helper_win_->firewallOn(connectingIp, ipStr, bAllowLanTraffic, bIsCustomConfig);
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

bool FirewallController_win::whitelistPorts(const api_responses::StaticIpPortsVector &ports)
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

void FirewallController_win::setFirewallOnBoot(bool bEnable, const QSet<QString> &ipTable, bool isAllowLanTraffic)
{
    Q_UNUSED(bEnable);
    //nothing todo for Windows
}
