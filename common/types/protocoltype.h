#ifndef TYPES_PROTOCOLTYPE_H
#define TYPES_PROTOCOLTYPE_H

#include <QString>
#include "utils/protobuf_includes.h"

namespace types {

class ProtocolType
{
public:
    enum PROTOCOL_TYPE { PROTOCOL_UNINITIALIZED, PROTOCOL_IKEV2, PROTOCOL_OPENVPN_UDP, PROTOCOL_OPENVPN_TCP,
                         PROTOCOL_STUNNEL, PROTOCOL_WSTUNNEL, PROTOCOL_WIREGUARD };

    ProtocolType();
    ProtocolType(PROTOCOL_TYPE protocol);  // cppcheck-suppress noExplicitConstructor
    ProtocolType(const ProtoTypes::Protocol &p);  // cppcheck-suppress noExplicitConstructor
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

    ProtoTypes::Protocol convertToProtobuf() const;


private:
    PROTOCOL_TYPE protocol_;
};

} //namespace types


#endif // TYPES_PROTOCOLTYPE_H
