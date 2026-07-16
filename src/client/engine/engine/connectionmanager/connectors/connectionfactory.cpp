#include "connectionfactory.h"

#include "engine/connectionmanager/connectors/openvpn/openvpnconnection.h"
#include "utils/ws_assert.h"

#ifdef Q_OS_WIN
    #include "engine/connectionmanager/connectors/ikev2/ikev2connection_win.h"
    #include "engine/connectionmanager/connectors/wireguard/wireguardconnection_win.h"
#elif defined Q_OS_MACOS
    #include "engine/connectionmanager/connectors/ikev2/ikev2connection_mac.h"
    #include "engine/connectionmanager/connectors/wireguard/wireguardconnection_posix.h"
#elif defined Q_OS_LINUX
    #include "engine/connectionmanager/connectors/ikev2/ikev2connection_linux.h"
    #include "engine/connectionmanager/connectors/wireguard/wireguardconnection_posix.h"
#endif

IConnection *ConnectionFactory::createConnection(types::Protocol protocol, QObject *parent, Helper *helper)
{
    if (protocol.isWireGuardProtocol()) {
        return new WireGuardConnection(parent, helper);
    } else if (protocol.isOpenVpnProtocol()) {
        return new OpenVPNConnection(parent, helper);
    } else if (protocol.isIkev2Protocol()) {
#ifdef Q_OS_WIN
        return new IKEv2Connection_win(parent, helper);
#elif defined Q_OS_MACOS
        return new IKEv2Connection_mac(parent, helper);
#elif defined Q_OS_LINUX
        return new IKEv2Connection_linux(parent, helper);
#endif
    }

    WS_ASSERT(false);
    return nullptr;
}

void ConnectionFactory::removeIkev2ConnectionFromOS()
{
#ifdef Q_OS_WIN
    IKEv2Connection_win::removeIkev2ConnectionFromOS();
#elif defined Q_OS_MACOS
    IKEv2Connection_mac::removeIkev2ConnectionFromOS();
#endif
}
