#include "protocol.h"

#include <QMetaType>

#include "utils/ws_assert.h"

const int typeIdProtocol = qRegisterMetaType<types::Protocol>("types::Protocol");

namespace types {

QString Protocol::toShortString() const
{
    if (value_ == OPENVPN_UDP || value_ == OPENVPN_TCP || value_ == STUNNEL
        || value_ == WSTUNNEL) {
        return "openvpn";
    }
    else if (value_ == IKEV2) {
        return "ikev2";
    }
    else if (value_ == WIREGUARD) {
        return "wireguard";
    }
    else {
        WS_ASSERT(false);
        return "unknown";
    }
}

QString Protocol::toLongString() const
{
    if (value_ == IKEV2) {
        return "IKEv2";
    }
    else if (value_ == OPENVPN_UDP)  {
        return "UDP";
    }
    else if (value_ == OPENVPN_TCP)  {
        return "TCP";
    }
    else if (value_ == STUNNEL)  {
        return "Stealth";
    }
    else if (value_ == WSTUNNEL) {
        return "WStunnel";
    }
    else if (value_ == WIREGUARD)  {
        return "WireGuard";
    }
    else
    {
        WS_ASSERT(false);
        return "unknown";
    }
}

bool Protocol::isOpenVpnProtocol() const
{
    return value_ == OPENVPN_UDP || value_ == OPENVPN_TCP ||
            value_ == STUNNEL || value_ == WSTUNNEL;
}

bool Protocol::isStunnelOrWStunnelProtocol() const
{
    return value_ == STUNNEL || value_ == WSTUNNEL;
}

bool Protocol::isIkev2Protocol() const
{
    return value_ == IKEV2;
}

bool Protocol::isWireGuardProtocol() const
{
    return value_ == WIREGUARD;
}

bool Protocol::isValid() const
{
    return value_ != UNINITIALIZED;
}

Protocol Protocol::fromString(const QString &strProtocol)
{
    if (strProtocol.compare("UDP", Qt::CaseInsensitive) == 0) {
        return OPENVPN_UDP;
    }
    else if (strProtocol.compare("TCP", Qt::CaseInsensitive) == 0) {
        return OPENVPN_TCP;
    }
    else if (strProtocol.compare("Stealth", Qt::CaseInsensitive) == 0) {
        return STUNNEL;
    }
    else if (strProtocol.compare("WStunnel", Qt::CaseInsensitive) == 0) {
        return WSTUNNEL;
    }
    else if (strProtocol.compare("WireGuard", Qt::CaseInsensitive) == 0) {
        return WIREGUARD;
    }
    else if (strProtocol.compare("IKEv2", Qt::CaseInsensitive) == 0) {
        return IKEV2;
    }
    else
    {
        return UNINITIALIZED;
    }
}

// return supported protocols depending on the OS in the preferred order of use
QList<Protocol> types::Protocol::supportedProtocols()
{
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    return QList<Protocol>() << WIREGUARD << IKEV2 << OPENVPN_UDP << OPENVPN_TCP << STUNNEL << WSTUNNEL;
#elif defined(Q_OS_LINUX)
    // Currently Linux doesn't support IKEv2
    return QList<Protocol>() << WIREGUARD << OPENVPN_UDP << OPENVPN_TCP << STUNNEL << WSTUNNEL;
#endif
}

uint Protocol::defaultPortForProtocol(Protocol protocol)
{
    if (protocol == IKEV2) return 500;
    else return 443;
}

QDataStream& operator <<(QDataStream &stream, const Protocol &o)
{
    stream << o.toInt();
    return stream;
}

QDataStream& operator >>(QDataStream &stream, Protocol &o)
{
    int v;
    stream >> v;
    o = (types::Protocol)v;
    return stream;
}

} // types namespace
