#include "protocoltype.h"
#include <QObject>

const int typeIdProtocolType = qRegisterMetaType<types::ProtocolType>("types::ProtocolType");

namespace types {

ProtocolType::ProtocolType() : protocol_(PROTOCOL_UNINITIALIZED)
{

}

ProtocolType::ProtocolType(PROTOCOL_TYPE protocol) : protocol_(protocol)
{

}

ProtocolType::ProtocolType(const QString &strProtocol)
{
    if (strProtocol.compare("UDP", Qt::CaseInsensitive) == 0)
    {
        protocol_ = PROTOCOL_OPENVPN_UDP;
    }
    else if (strProtocol.compare("TCP", Qt::CaseInsensitive) == 0)
    {
        protocol_ = PROTOCOL_OPENVPN_TCP;
    }
    else if (strProtocol.compare("Stealth", Qt::CaseInsensitive) == 0)
    {
        protocol_ = PROTOCOL_STUNNEL;
    }
    else if (strProtocol.compare("WStunnel", Qt::CaseInsensitive) == 0)
    {
        protocol_ = PROTOCOL_WSTUNNEL;
    }
    else if (strProtocol.compare("WireGuard", Qt::CaseInsensitive) == 0)
    {
        protocol_ = PROTOCOL_WIREGUARD;
    }
    else if (strProtocol.compare("IKEv2", Qt::CaseInsensitive) == 0)
    {
        protocol_ = PROTOCOL_IKEV2;
    }
    else
    {
        Q_ASSERT(false);
        protocol_ = PROTOCOL_UNINITIALIZED;
    }
}

ProtocolType::PROTOCOL_TYPE ProtocolType::getType() const
{
    return protocol_;
}

bool ProtocolType::isInitialized() const
{
    return protocol_ != PROTOCOL_UNINITIALIZED;
}

bool ProtocolType::isOpenVpnProtocol() const
{
    Q_ASSERT(protocol_ != PROTOCOL_UNINITIALIZED);
    return protocol_ == PROTOCOL_OPENVPN_UDP || protocol_ == PROTOCOL_OPENVPN_TCP ||
           protocol_ == PROTOCOL_STUNNEL || protocol_ == PROTOCOL_WSTUNNEL;
}

bool ProtocolType::isIkev2Protocol() const
{
    Q_ASSERT(protocol_ != PROTOCOL_UNINITIALIZED);
    return protocol_ == PROTOCOL_IKEV2;
}

bool ProtocolType::isWireGuardProtocol() const
{
    Q_ASSERT(protocol_ != PROTOCOL_UNINITIALIZED);
    return protocol_ == PROTOCOL_WIREGUARD;
}

bool ProtocolType::isStunnelOrWStunnelProtocol() const
{
    Q_ASSERT(protocol_ != PROTOCOL_UNINITIALIZED);
    return protocol_ == PROTOCOL_STUNNEL || protocol_ == PROTOCOL_WSTUNNEL;
}

bool ProtocolType::isEqual(const ProtocolType &other) const
{
    return protocol_ == other.protocol_;
}

QString ProtocolType::toShortString() const
{
    //todo check
    Q_ASSERT(protocol_ != PROTOCOL_UNINITIALIZED);
    if (protocol_ == PROTOCOL_OPENVPN_UDP || protocol_ == PROTOCOL_OPENVPN_TCP || protocol_ == PROTOCOL_STUNNEL
        || protocol_ == PROTOCOL_WSTUNNEL)
    {
        return "openvpn";
    }
    else if (protocol_ == PROTOCOL_IKEV2)
    {
        return "ikev2";
    }
    else
    {
        return "uninitialized";
    }
}

QString ProtocolType::toLongString() const
{
    Q_ASSERT(protocol_ != PROTOCOL_UNINITIALIZED);
    if (protocol_ == PROTOCOL_IKEV2)
    {
        return "IKEv2";
    }
    else if (protocol_ == PROTOCOL_OPENVPN_UDP)
    {
        return "UDP";
    }
    else if (protocol_ == PROTOCOL_OPENVPN_TCP)
    {
        return "TCP";
    }
    else if (protocol_ == PROTOCOL_STUNNEL)
    {
        return "Stealth";
    }
    else if (protocol_ == PROTOCOL_WSTUNNEL)
    {
        return "WStunnel";
    }
    else if (protocol_ == PROTOCOL_WIREGUARD)
    {
        return "WireGuard";
    }
    else
    {
        return "uninitialized";
    }
}

} //namespace types
