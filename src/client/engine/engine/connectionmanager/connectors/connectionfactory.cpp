#include "connectionfactory.h"

#include "engine/connectionmanager/connectrequest.h"
#include "engine/connectionmanager/connectors/openvpn/openvpnconnection.h"
#include "engine/wireguardconfig/getwireguardconfig.h"
#include "utils/ws_assert.h"

#ifdef Q_OS_WIN
    #include <windows.h>
    #include "engine/connectionmanager/connectors/ikev2/ikev2connection_win.h"
    #include "engine/connectionmanager/connectors/wireguard/wireguardconnection_win.h"
#elif defined Q_OS_MACOS
    #include "engine/connectionmanager/connectors/ikev2/ikev2connection_mac.h"
    #include "engine/connectionmanager/connectors/wireguard/wireguardconnection_posix.h"
#elif defined Q_OS_LINUX
    #include "engine/connectionmanager/connectors/ikev2/ikev2connection_linux.h"
    #include "engine/connectionmanager/connectors/wireguard/wireguardconnection_posix.h"
#endif

IConnection *ConnectionFactory::createConnection(types::Protocol protocol, QObject *parent, const ConnectRequest &request)
{
    if (protocol.isWireGuardProtocol()) {
        return new WireGuardConnection(parent, helper_, protocol, request.wireGuard);
    } else if (protocol.isOpenVpnProtocol()) {
        return new OpenVPNConnection(parent, helper_, protocol, request.openVpn);
    } else if (protocol.isIkev2Protocol()) {
#ifdef Q_OS_WIN
        return new IKEv2Connection_win(parent, helper_, protocol, request.ikev2);
#elif defined Q_OS_MACOS
        return new IKEv2Connection_mac(parent, helper_, protocol, request.ikev2);
#elif defined Q_OS_LINUX
        return new IKEv2Connection_linux(parent, helper_, protocol, request.ikev2);
#endif
    }

    WS_ASSERT(false);
    return nullptr;
}

void ConnectionFactory::removeIkev2ConnectionFromOS()
{
#ifdef Q_OS_WIN
    IKEv2Connection_win::removeIkev2ConnectionFromOS(helper_);
#elif defined Q_OS_MACOS
    IKEv2Connection_mac::removeIkev2ConnectionFromOS();
#endif
}

bool ConnectionFactory::hasUsableStoredConfig() const
{
    return GetWireGuardConfig::hasUsableStoredConfig();
}

void ConnectionFactory::removeStoredConfig()
{
    GetWireGuardConfig::removeWireGuardSettings();
}

void ConnectionFactory::finishActiveConnections(Helper *helper)
{
#ifdef Q_OS_WIN
    helper->executeTaskKill(kTargetOpenVpn);

    const QVector<HRASCONN> v = IKEv2Connection_win::getActiveIkev2Connections(helper);
    if (!v.isEmpty()) {
        for (HRASCONN hRas : v) {
            IKEv2Connection_win::blockingDisconnect(hRas);
        }

        helper->removeHosts();
    }

    helper->stopWireGuard();
    helper->disableDnsLeaksProtection();
#else
    helper->executeTaskKill(kTargetOpenVpn);
    helper->executeTaskKill(kTargetStunnel);
    helper->executeTaskKill(kTargetWStunnel);
    helper->stopWireGuard();
#ifdef Q_OS_MACOS
    IKEv2Connection_mac::closeAppActiveConnection();
#endif
#endif
}
