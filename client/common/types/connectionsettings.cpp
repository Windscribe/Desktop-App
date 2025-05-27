#include "connectionsettings.h"
#include "types/enums.h"
#include "utils/log/categories.h"
#include "utils/ws_assert.h"

const int typeIdConnectionSettings = qRegisterMetaType<types::ConnectionSettings>("types::ConnectionSettings");

namespace types {

// init with default connection settings
ConnectionSettings::ConnectionSettings()
{
    // take the first supported protocol on this OS (currenty it's WireGuard 443)
    QList<Protocol> protocols = Protocol::supportedProtocols();
    WS_ASSERT(!protocols.isEmpty());
    protocol_ = protocols[0];
    port_ = Protocol::defaultPortForProtocol(protocol_);
}

ConnectionSettings::ConnectionSettings(Protocol protocol, uint port, bool isAutomatic) :
    protocol_(protocol), port_(port), isAutomatic_(isAutomatic)
{
    checkForUnavailableProtocolAndFix();
}

ConnectionSettings::ConnectionSettings(const QJsonObject &json)
{
    if (json.contains(kJsonProtocolProp) && json[kJsonProtocolProp].isDouble()) {
        protocol_ = Protocol(json[kJsonProtocolProp].toInt());
    } else {
        QList<Protocol> protocols = Protocol::supportedProtocols();
        protocol_ = protocols[0];
    }

    if (json.contains(kJsonPortProp) && json[kJsonPortProp].isDouble()) {
        uint port = static_cast<uint>(json[kJsonPortProp].toInt());
        if (port >= 0 && port < 65536) {
            port_ = port;
        } else {
            port_ = Protocol::defaultPortForProtocol(protocol_);
        }
    } else {
        port_ = Protocol::defaultPortForProtocol(protocol_);
    }

    if (json.contains(kJsonIsAutomaticProp) && json[kJsonIsAutomaticProp].isBool()) {
        isAutomatic_ = json[kJsonIsAutomaticProp].toBool();
    }

    checkForUnavailableProtocolAndFix();
}

ConnectionSettings::ConnectionSettings(QSettings &settings, const QString &key)
{
    fromIni(settings, key);
}

void ConnectionSettings::setProtocolAndPort(Protocol protocol, uint port)
{
    protocol_ = protocol;
    port_ = port;
    checkForUnavailableProtocolAndFix();
}

void ConnectionSettings::setPort(uint port)
{
    port_ = port;
}

void ConnectionSettings::setIsAutomatic(bool isAutomatic)
{
    isAutomatic_ = isAutomatic;
}

QJsonObject ConnectionSettings::toJson(bool isForDebugLog) const
{
    QJsonObject json;
    json[kJsonProtocolProp] = protocol_.toInt();
    json[kJsonPortProp] = static_cast<int>(port_);
    json[kJsonIsAutomaticProp] = isAutomatic_;
    if (isForDebugLog) {
        json["protocolDesc"] = protocol_.toLongString();
    }
    return json;
}

void ConnectionSettings::fromIni(QSettings &settings, const QString &key)
{
    QString prevMode = TOGGLE_MODE_toString(isAutomatic_ ? TOGGLE_MODE_AUTO : TOGGLE_MODE_MANUAL);

    if (!key.isEmpty()) {
        settings.beginGroup(key);
    }

    TOGGLE_MODE mode = TOGGLE_MODE_fromString(settings.value(kIniIsAutomaticProp, prevMode).toString());
    isAutomatic_ = (mode == TOGGLE_MODE_AUTO);
    protocol_ = Protocol::fromString(settings.value(kIniProtocolProp, protocol_.toLongString()).toString());
    uint port = settings.value(kIniPortProp, port_).toUInt();
    if (port < 65536) {
        port_ = port;
    } else {
        port_ = Protocol::defaultPortForProtocol(protocol_);
    }

    if (!key.isEmpty()) {
        settings.endGroup();
    }

    checkForUnavailableProtocolAndFix();
}

void ConnectionSettings::toIni(QSettings &settings, const QString &key) const
{
    if (!key.isEmpty()) {
        settings.beginGroup(key);
    }

    settings.setValue(kIniIsAutomaticProp, TOGGLE_MODE_toString(isAutomatic_ ? TOGGLE_MODE_AUTO : TOGGLE_MODE_MANUAL));
    settings.setValue(kIniProtocolProp, protocol_.toLongString());
    settings.setValue(kIniPortProp, port_);

    if (!key.isEmpty()) {
        settings.endGroup();
    }
}

void ConnectionSettings::checkForUnavailableProtocolAndFix()
{
    QList<Protocol> protocols = Protocol::supportedProtocols();
    WS_ASSERT(!protocols.isEmpty());
    if (!protocols.contains(protocol_)) {
        protocol_ = protocols[0];
        port_ = Protocol::defaultPortForProtocol(protocol_);
    }
}

QDataStream& operator <<(QDataStream &stream, const ConnectionSettings &o)
{
    stream << o.versionForSerialization_;
    stream << o.protocol_ << o.port_ << o.isAutomatic_;
    return stream;
}

QDataStream& operator >>(QDataStream &stream, ConnectionSettings &o)
{
    quint32 version;
    stream >> version;
    if (version > o.versionForSerialization_)
    {
        stream.setStatus(QDataStream::ReadCorruptData);
        return stream;
    }
    stream >> o.protocol_ >> o.port_ >> o.isAutomatic_;
    o.checkForUnavailableProtocolAndFix();
    return stream;
}

} //namespace types
