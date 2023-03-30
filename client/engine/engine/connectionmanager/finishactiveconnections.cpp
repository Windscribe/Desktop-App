#include "finishactiveconnections.h"
#include "../openvpnversioncontroller.h"
#include "utils/logger.h"

#ifdef Q_OS_WIN
    #include <windows.h>
    #include "ikev2connection_win.h"
    #include "wireguardconnection_win.h"
#endif

#ifdef Q_OS_MAC
    #include "restorednsmanager_mac.h"
    #include "ikev2connection_mac.h"
    #include "engine/helper/helper_mac.h"
    #include "wireguardconnection_posix.h"
#endif

#ifdef Q_OS_LINUX
    #include "engine/helper/helper_linux.h"
    #include "wireguardconnection_posix.h"
#endif

void FinishActiveConnections::finishAllActiveConnections(IHelper *helper)
{
#ifdef Q_OS_WIN
    finishAllActiveConnections_win(helper);
#else
    finishAllActiveConnections_posix(helper);
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
    helper_win->stopWireGuard();
    helper_win->disableDnsLeaksProtection();
}
#elif defined(Q_OS_MAC) || defined(Q_OS_LINUX)
void FinishActiveConnections::finishAllActiveConnections_posix(IHelper *helper)
{
#if defined(Q_OS_LINUX)
    removeDnsLeaksprotection_linux(helper);
#endif
    finishOpenVpnActiveConnections_posix(helper);
    finishWireGuardActiveConnections_posix(helper);
#if defined(Q_OS_MAC)
    IKEv2Connection_mac::closeWindscribeActiveConnection();
#endif
}

void FinishActiveConnections::finishOpenVpnActiveConnections_posix(IHelper *helper)
{
    Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper);
    helper_posix->executeTaskKill(kTargetOpenVpn);
    helper_posix->executeTaskKill(kTargetStunnel);
    helper_posix->executeTaskKill(kTargetWStunnel);
#if defined(Q_OS_MAC)
    RestoreDNSManager_mac::restoreState(helper);
#endif
}

void FinishActiveConnections::finishWireGuardActiveConnections_posix(IHelper *helper)
{
    helper->setDefaultWireGuardDeviceName(WireGuardConnection::getWireGuardAdapterName());
    helper->stopWireGuard();
}
#endif

#if defined(Q_OS_LINUX)
void FinishActiveConnections::removeDnsLeaksprotection_linux(IHelper *helper)
{
    Helper_linux *helperLinux = dynamic_cast<Helper_linux *>(helper);
    helperLinux->setDnsLeakProtectEnabled(false);
}
#endif



