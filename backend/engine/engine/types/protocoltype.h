#ifndef PROTOCOLTYPE_H
#define PROTOCOLTYPE_H

#include <QString>
#include "ipc/generated_proto/types.pb.h"

class ProtocolType
{
public:
    enum PROTOCOL_TYPE { PROTOCOL_UNINITIALIZED, PROTOCOL_IKEV2, PROTOCOL_OPENVPN_UDP, PROTOCOL_OPENVPN_TCP,
                         PROTOCOL_STUNNEL, PROTOCOL_WSTUNNEL };

    ProtocolType();
    ProtocolType(PROTOCOL_TYPE protocol);
    ProtocolType(const ProtoTypes::Protocol &p);
    ProtocolType(const QString &strProtocol);
    PROTOCOL_TYPE getType() const;
    bool isInitialized() const;
    bool isOpenVpnProtocol() const;
    bool isIkev2Protocol() const;
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

#endif // PROTOCOLTYPE_H
