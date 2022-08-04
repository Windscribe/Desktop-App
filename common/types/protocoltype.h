#ifndef TYPES_PROTOCOLTYPE_H
#define TYPES_PROTOCOLTYPE_H

#include <QString>
#include <QDataStream>

namespace types {

class ProtocolType
{
public:
    enum PROTOCOL_TYPE { PROTOCOL_UNINITIALIZED = 100000, PROTOCOL_IKEV2 = 0, PROTOCOL_OPENVPN_UDP = 1, PROTOCOL_OPENVPN_TCP = 2,
                         PROTOCOL_STUNNEL = 3, PROTOCOL_WSTUNNEL = 4, PROTOCOL_WIREGUARD = 5};

    ProtocolType();
    ProtocolType(PROTOCOL_TYPE protocol);  // cppcheck-suppress noExplicitConstructor
    ProtocolType(const QString &strProtocol);  // cppcheck-suppress noExplicitConstructor
    PROTOCOL_TYPE getType() const;
    bool isInitialized() const;
    bool isOpenVpnProtocol() const;
    bool isIkev2Protocol() const;
    bool isWireGuardProtocol() const;
    bool isStunnelOrWStunnelProtocol() const;
    bool isEqual(const ProtocolType &other) const;
    // return ikev2/openvpn string
    QString toShortString() const;
    // return ikev2/udp/tcp/stealth string
    QString toLongString() const;

    bool operator==(const ProtocolType &other) const
    {
        return other.protocol_ == protocol_;
    }

    bool operator!=(const ProtocolType &other) const
    {
        return !(*this == other);
    }

    friend QDataStream& operator <<(QDataStream& stream, const ProtocolType& p)
    {
        stream << versionForSerialization_;
        stream << static_cast<int>(p.protocol_);
        return stream;
    }
    friend QDataStream& operator >>(QDataStream& stream, ProtocolType& p)
    {
        quint32 version;
        stream >> version;
        Q_ASSERT(version == versionForSerialization_);
        int i;
        stream >> i;
        p.protocol_ = static_cast<PROTOCOL_TYPE>(i);
        return stream;
    }

private:
    PROTOCOL_TYPE protocol_;
    static constexpr quint32 versionForSerialization_ = 1;
};

} //namespace types


#endif // TYPES_PROTOCOLTYPE_H
