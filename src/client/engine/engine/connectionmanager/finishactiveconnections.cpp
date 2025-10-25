#include "finishactiveconnections.h"
#include "../openvpnversioncontroller.h"
#include "utils/log/logger.h"

#ifdef Q_OS_WIN
    #include <windows.h>
    #include "ikev2connection_win.h"
    #include "wireguardconnection_win.h"
#endif

#ifdef Q_OS_MACOS
    #include "restorednsmanager_mac.h"
    #include "ikev2connection_mac.h"
    #include "wireguardconnection_mac.h"
#endif

#ifdef Q_OS_LINUX
    #include "wireguardconnection_linux.h"
#endif

void FinishActiveConnections::finishAllActiveConnections(Helper *helper)
{
#ifdef Q_OS_WIN
    finishAllActiveConnections_win(helper);
#else
    finishAllActiveConnections_posix(helper);
#endif
}

#ifdef Q_OS_WIN
void FinishActiveConnections::finishAllActiveConnections_win(Helper *helper)
{
    finishOpenVpnActiveConnections_win(helper);
    finishIkev2ActiveConnections_win(helper);
    finishWireGuardActiveConnections_win(helper);
}

void FinishActiveConnections::finishOpenVpnActiveConnections_win(Helper *helper)
{
    helper->executeTaskKill(kTargetOpenVpn);
}

void FinishActiveConnections::finishIkev2ActiveConnections_win(Helper *helper)
{
    const QVector<HRASCONN> v = IKEv2Connection_win::getActiveWindscribeConnections();

    if (!v.isEmpty())
    {
        for (HRASCONN hRas : v)
        {
            IKEv2ConnectionDisconnectLogic_win::blockingDisconnect(hRas);
        }

        helper->disableDnsLeaksProtection();
        helper->removeHosts();
    }
}

void FinishActiveConnections::finishWireGuardActiveConnections_win(Helper *helper)
{
    helper->stopWireGuard();
    helper->disableDnsLeaksProtection();
}
#elif defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
void FinishActiveConnections::finishAllActiveConnections_posix(Helper *helper)
{
#if defined(Q_OS_LINUX)
    removeDnsLeaksprotection_linux(helper);
#endif
    finishOpenVpnActiveConnections_posix(helper);
    finishWireGuardActiveConnections_posix(helper);
#if defined(Q_OS_MACOS)
    IKEv2Connection_mac::stopWindscribeActiveConnection();
    WireGuardConnection::stopWindscribeActiveConnection();
#endif
}

void FinishActiveConnections::finishOpenVpnActiveConnections_posix(Helper *helper)
{
    helper->executeTaskKill(kTargetOpenVpn);
    helper->executeTaskKill(kTargetStunnel);
    helper->executeTaskKill(kTargetWStunnel);
#if defined(Q_OS_MACOS)
    RestoreDNSManager_mac::restoreState(helper);
#endif
}

void FinishActiveConnections::finishWireGuardActiveConnections_posix(Helper *helper)
{
#if defined(Q_OS_LINUX)
    helper->stopWireGuard();
#endif
}
#endif

#if defined(Q_OS_LINUX)
void FinishActiveConnections::removeDnsLeaksprotection_linux(Helper *helper)
{
    helper->setDnsLeakProtectEnabled(false);
}
#endif



