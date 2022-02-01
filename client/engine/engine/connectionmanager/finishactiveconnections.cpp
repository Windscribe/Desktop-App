#include "finishactiveconnections.h"
#include "../openvpnversioncontroller.h"
#include "wireguardconnection.h"
#include "utils/logger.h"

#ifdef Q_OS_WIN
    #include <windows.h>
    #include "ikev2connection_win.h"
#endif

#ifdef Q_OS_MAC
    #include "restorednsmanager_mac.h"
    #include "ikev2connection_mac.h"
    #include "engine/helper/helper_mac.h"
#endif

#ifdef Q_OS_LINUX
    #include "engine/helper/helper_posix.h"
#endif

void FinishActiveConnections::finishAllActiveConnections(IHelper *helper)
{
#ifdef Q_OS_WIN
    finishAllActiveConnections_win(helper);
#elif defined Q_OS_MAC
    finishAllActiveConnections_mac(helper);
#elif defined Q_OS_LINUX
    finishAllActiveConnections_linux(helper);
#endif
}

#ifdef Q_OS_WIN
void FinishActiveConnections::finishAllActiveConnections_win(IHelper *helper)
{
    finishOpenVpnActiveConnections_win(helper);
    finishIkev2ActiveConnections_win(helper);
    finishWireGuardActiveConnections_win(helper);
}

void FinishActiveConnections::finishOpenVpnActiveConnections_win(IHelper *helper)
{
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper);
    const QStringList strOpenVpnExeList = OpenVpnVersionController::instance().getAvailableOpenVpnExecutables();
    for (const QString &strExe : strOpenVpnExeList)
    {
        helper_win->executeTaskKill(strExe);
    }
}

void FinishActiveConnections::finishIkev2ActiveConnections_win(IHelper *helper)
{
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper);
    const QVector<HRASCONN> v = IKEv2Connection_win::getActiveWindscribeConnections();

    if (!v.isEmpty())
    {
        for (HRASCONN hRas : v)
        {
            IKEv2ConnectionDisconnectLogic_win::blockingDisconnect(hRas);
        }

        helper_win->disableDnsLeaksProtection();
        helper_win->removeHosts();
    }
}

void FinishActiveConnections::finishWireGuardActiveConnections_win(IHelper *helper)
{
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper);
    QString strWireGuardExe = WireGuardConnection::getWireGuardExeName();
    if (!strWireGuardExe.endsWith(".exe"))
        strWireGuardExe.append(".exe");
    helper_win->executeTaskKill(strWireGuardExe);
    helper_win->stopWireGuard();  // This will also reset route monitoring.
}
#elif defined Q_OS_MAC
void FinishActiveConnections::finishAllActiveConnections_mac(IHelper *helper)
{
    finishOpenVpnActiveConnections_mac(helper);
    finishWireGuardActiveConnections_mac(helper);
    IKEv2Connection_mac::closeWindscribeActiveConnection();
}

void FinishActiveConnections::finishOpenVpnActiveConnections_mac(IHelper *helper)
{
    Helper_mac *helper_mac = dynamic_cast<Helper_mac *>(helper);
    const QStringList strOpenVpnExeList = OpenVpnVersionController::instance().getAvailableOpenVpnExecutables();
    for (const QString &strExe : strOpenVpnExeList)
    {
        helper_mac->executeTaskKill(strExe);
    }
    helper_mac->executeTaskKill("windscribestunnel");
    helper_mac->executeTaskKill("windscribewstunnel");
    RestoreDNSManager_mac::restoreState(helper);
}
void FinishActiveConnections::finishWireGuardActiveConnections_mac(IHelper *helper)
{
    helper->setDefaultWireGuardDeviceName(WireGuardConnection::getWireGuardAdapterName());
    helper->stopWireGuard();
}
#elif defined Q_OS_LINUX

void FinishActiveConnections::finishAllActiveConnections_linux(IHelper *helper)
{
    // todo: kill openvpn, wireguard for Linux
    removeDnsLeaksprotection_linux(helper);
}

void FinishActiveConnections::removeDnsLeaksprotection_linux(IHelper *helper)
{
    Helper_posix *helperPosix = dynamic_cast<Helper_posix *>(helper);
    int exitCode;
    helperPosix->executeRootCommand("/etc/windscribe/dns-leak-protect down", &exitCode);
}

#endif



