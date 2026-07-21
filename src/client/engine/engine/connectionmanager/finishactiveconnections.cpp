#include "finishactiveconnections.h"
#include "utils/log/logger.h"

#ifdef Q_OS_WIN
    #include <windows.h>
    #include "engine/connectionmanager/connectors/ikev2/ikev2connection_win.h"
    #include "engine/connectionmanager/connectors/wireguard/wireguardconnection_win.h"
    #include "engine/dns/ctrldmanager/ctrldmanager_win.h"
#endif

#ifdef Q_OS_MACOS
    #include "engine/connectionmanager/connectors/ikev2/ikev2connection_mac.h"
    #include "engine/connectionmanager/connectors/wireguard/wireguardconnection_posix.h"
    #include "engine/dns/restorednsmanager_mac.h"
#endif

#ifdef Q_OS_LINUX
    #include "engine/connectionmanager/connectors/wireguard/wireguardconnection_posix.h"
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
    CtrldManager_win::terminateAppCtrldProcesses();
}

void FinishActiveConnections::finishOpenVpnActiveConnections_win(Helper *helper)
{
    helper->executeTaskKill(kTargetOpenVpn);
}

void FinishActiveConnections::finishIkev2ActiveConnections_win(Helper *helper)
{
    const QVector<HRASCONN> v = IKEv2Connection_win::getActiveIkev2Connections();

    if (!v.isEmpty())
    {
        for (HRASCONN hRas : v)
        {
            IKEv2Connection_win::blockingDisconnect(hRas);
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
    IKEv2Connection_mac::closeAppActiveConnection();
#endif
    helper->executeTaskKill(kTargetCtrld);
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
    helper->stopWireGuard();
}
#endif

#if defined(Q_OS_LINUX)
void FinishActiveConnections::removeDnsLeaksprotection_linux(Helper *helper)
{
    helper->setDnsLeakProtectEnabled(false);
    helper->setGaiIpv4PriorityEnabled(false);
}
#endif



