#ifndef TYPES_CONNECTIONSETTINGS_H
#define TYPES_CONNECTIONSETTINGS_H

#include <QJsonObject>
#include <QSettings>
#include <QString>
#include "protocol.h"

namespace types {

struct ConnectionSettings
{
    ConnectionSettings();
    explicit ConnectionSettings(Protocol protocol, uint port, bool isAutomatic);

    Protocol protocol() const { return protocol_; }
    uint port() const { return port_; }
    bool isAutomatic() const { return isAutomatic_; }

    void setProtocolAndPort(Protocol protocol, uint port);
    void setPort(uint port);
    void setIsAutomatic(bool isAutomatic);

    bool operator==(const ConnectionSettings &other) const
    {
        return other.protocol_ == protocol_ &&
               other.port_ == port_ &&
               other.isAutomatic_ == isAutomatic_;
    }

    bool operator!=(const ConnectionSettings &other) const
    {
        return !(*this == other);
    }

    friend QDataStream& operator <<(QDataStream &stream, const ConnectionSettings &o);
    friend QDataStream& operator >>(QDataStream &stream, ConnectionSettings &o);

    friend QDebug operator<<(QDebug dbg, const ConnectionSettings &cs);

    // if the currently settled protocol is not supported on this OS, then replace it.
    void checkForUnavailableProtocolAndFix();

private:
    Protocol protocol_;
    uint    port_;
    bool    isAutomatic_;

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} //namespace types

#endif // TYPES_CONNECTIONSETTINGS_H
