#ifndef TYPES_CONNECTIONSETTINGS_H
#define TYPES_CONNECTIONSETTINGS_H

#include <QJsonObject>
#include <QSettings>
#include <QString>
#include "protocoltype.h"

namespace types {

struct ConnectionSettings
{
    ConnectionSettings();
    void set(const ProtocolType &protocol, uint port, bool bAutomatic);
    void set(const ProtocolType &protocol, uint port);
    void set(const ConnectionSettings &s);
    void setPort(uint port);

    ProtocolType protocol() const;
    void setProtocol(ProtocolType protocol);

    uint port() const;
    bool isAutomatic() const;
    void setIsAutomatic(bool isAutomatic);

    bool readFromSettingsV1(QSettings &settings);

    void debugToLog() const;
    void logConnectionSettings() const;

    bool operator==(const ConnectionSettings &other) const
    {
        return other.protocol_ == protocol_ &&
               other.port_ == port_ &&
               other.bAutomatic_ == bAutomatic_;
    }

    bool operator!=(const ConnectionSettings &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJsonObject() const
    {
        QJsonObject json;
        json["protocol"] = (int)protocol_.getType();
        json["port"] = (qint64)port_;
        json["isAutomatic"] = bAutomatic_;
        return json;
    }

    bool fromJsonObject(const QJsonObject &json)
    {
        if (json.contains("protocol")) protocol_ = (ProtocolType::PROTOCOL_TYPE)json["protocol"].toInt(ProtocolType::PROTOCOL_IKEV2);
        if (json.contains("port")) port_ = json["port"].toInteger(500);
        if (json.contains("isAutomatic")) bAutomatic_ = json["isAutomatic"].toBool(true);
        return true;
    }


private:
    ProtocolType protocol_;
    uint    port_;
    bool    bAutomatic_;

    static constexpr quint32 versionForSerialization_ = 1;
};

} //namespace types

#endif // TYPES_CONNECTIONSETTINGS_H
