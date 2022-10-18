#ifndef TYPES_CONNECTIONSETTINGS_H
#define TYPES_CONNECTIONSETTINGS_H

#include <QJsonObject>
#include <QSettings>
#include <QString>
#include "protocol.h"

namespace types {

struct ConnectionSettings
{
    // FIXME: check where should be initialized?
    Protocol protocol = Protocol::WIREGUARD;
    uint    port = 443;
    bool    isAutomatic = true;

    void debugToLog() const;
    void logConnectionSettings() const;

    bool operator==(const ConnectionSettings &other) const
    {
        return other.protocol == protocol &&
               other.port == port &&
               other.isAutomatic == isAutomatic;
    }

    bool operator!=(const ConnectionSettings &other) const
    {
        return !(*this == other);
    }

    friend QDataStream& operator <<(QDataStream &stream, const ConnectionSettings &o);
    friend QDataStream& operator >>(QDataStream &stream, ConnectionSettings &o);

    friend QDebug operator<<(QDebug dbg, const ConnectionSettings &cs);

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed

};

} //namespace types

#endif // TYPES_CONNECTIONSETTINGS_H
