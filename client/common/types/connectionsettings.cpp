#include "connectionsettings.h"
#include "utils/logger.h"
#include "utils/ws_assert.h"

const int typeIdConnectionSettings = qRegisterMetaType<types::ConnectionSettings>("types::ConnectionSettings");

namespace types {

// init with default connection settings
ConnectionSettings::ConnectionSettings() :
    isAutomatic_(true)
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
    if (json.contains(jsonInfo_.kProtocolProp) && json[jsonInfo_.kProtocolProp].isDouble())
        protocol_ = static_cast<Protocol>(json[jsonInfo_.kProtocolProp].toInt());

    if (json.contains(jsonInfo_.kPortProp) && json[jsonInfo_.kPortProp].isDouble())
        port_ = static_cast<uint>(json[jsonInfo_.kPortProp].toInt());

    if (json.contains(jsonInfo_.kIsAutomaticProp) && json[jsonInfo_.kIsAutomaticProp].isBool())
        isAutomatic_ = json[jsonInfo_.kIsAutomaticProp].toBool();
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

QJsonObject ConnectionSettings::toJson() const
{
    QJsonObject json;
    json[jsonInfo_.kProtocolProp] = protocol_.toInt();
    json[jsonInfo_.kPortProp] = static_cast<int>(port_);
    json[jsonInfo_.kIsAutomaticProp] = isAutomatic_;
    return json;
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

QDebug operator<<(QDebug dbg, const ConnectionSettings &cs)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "{isAutomatic:" << cs.isAutomatic_ << "; ";
    dbg << "protocol:" << cs.protocol_.toLongString() << "; ";
    dbg << "port:" << cs.port_ << "}";
    return dbg;
}

} //namespace types

