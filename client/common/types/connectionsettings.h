#pragma once

#include <QJsonObject>
#include <QSettings>
#include <QString>
#include "protocol.h"

namespace types {

struct ConnectionSettings
{
    ConnectionSettings();
    explicit ConnectionSettings(Protocol protocol, uint port, bool isAutomatic);
    ConnectionSettings(const QJsonObject& jsonObject);
    ConnectionSettings(QSettings &settings, const QString &key = "");

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

    QJsonObject toJson(bool isForDebugLog) const;
    void fromIni(QSettings &settings, const QString &key = "");
    void toIni(QSettings &settings, const QString &key = "") const;

    friend QDataStream& operator <<(QDataStream &stream, const ConnectionSettings &o);
    friend QDataStream& operator >>(QDataStream &stream, ConnectionSettings &o);

    // if the currently settled protocol is not supported on this OS, then replace it.
    void checkForUnavailableProtocolAndFix();

private:
    Protocol protocol_;
    uint    port_ = 0;
    bool    isAutomatic_ = true;

    static const inline QString kIniProtocolProp = "ManualConnectionProtocol";
    static const inline QString kIniPortProp = "ManualConnectionPort";
    static const inline QString kIniIsAutomaticProp = "ConnectionMode";

    static const inline QString kJsonProtocolProp = "protocol";
    static const inline QString kJsonPortProp = "port";
    static const inline QString kJsonIsAutomaticProp = "isAutomatic";

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} //namespace types
