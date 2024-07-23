#pragma once
#include <QString>

namespace types {

class Protocol
{
public:
    enum TYPE {
        IKEV2 = 0,
        OPENVPN_UDP = 1,
        OPENVPN_TCP = 2,
        STUNNEL = 3,
        WSTUNNEL = 4,
        WIREGUARD = 5,
        UNINITIALIZED = 100000
    };

    Protocol() : value_(UNINITIALIZED) {}
    Protocol(TYPE a) : value_(a) {}
    Protocol(int a) {
        if (a >= 0 && a <= 5) {
            value_ = static_cast<TYPE>(a);
        } else {
            value_ = UNINITIALIZED;
        }
    }

    // Prevent usage: if(Protocol)
    explicit operator bool() const = delete;

    bool operator==(Protocol a) const { return value_ == a.value_; }
    bool operator!=(Protocol a) const { return value_ != a.value_; }

    // utils functions
    int toInt() const { return (int)value_; }
    QString toShortString() const;
    QString toLongString() const;
    bool isOpenVpnProtocol() const;
    bool isStunnelOrWStunnelProtocol() const;
    bool isIkev2Protocol() const;
    bool isWireGuardProtocol() const;
    bool isValid() const;

    static Protocol fromString(const QString &strProtocol);
    static QList<Protocol> supportedProtocols();
    static uint defaultPortForProtocol(Protocol protocol);

private:
    TYPE value_;
};

QDataStream& operator <<(QDataStream &stream, const Protocol &o);
QDataStream& operator >>(QDataStream &stream, Protocol &o);

} // types namespace
