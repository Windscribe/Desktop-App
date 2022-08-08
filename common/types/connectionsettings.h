#ifndef TYPES_CONNECTIONSETTINGS_H
#define TYPES_CONNECTIONSETTINGS_H

#include <QJsonObject>
#include <QSettings>
#include <QString>
#include "enums.h"

namespace types {

struct ConnectionSettings
{
    PROTOCOL protocol = PROTOCOL::IKEV2;
    uint    port = 500;
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

    QJsonObject toJsonObject() const
    {
        QJsonObject json;
        json["protocol"] = protocol.toInt();
        json["port"] = (qint64)port;
        json["isAutomatic"] = isAutomatic;
        return json;
    }

    bool fromJsonObject(const QJsonObject &json)
    {
        if (json.contains("protocol")) protocol = json["protocol"].toInt(PROTOCOL::IKEV2);
        if (json.contains("port")) port = json["port"].toInteger(500);
        if (json.contains("isAutomatic")) isAutomatic = json["isAutomatic"].toBool(true);
        return true;
    }

    friend QDebug operator<<(QDebug dbg, const ConnectionSettings &cs);

};

} //namespace types

#endif // TYPES_CONNECTIONSETTINGS_H
