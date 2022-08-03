#include "connectionsettings.h"
#include "utils/logger.h"

const int typeIdConnectionSettings = qRegisterMetaType<types::ConnectionSettings>("types::ConnectionSettings");

namespace types {

ConnectionSettings::ConnectionSettings() :
    protocol_(ProtocolType::PROTOCOL_IKEV2), port_(500), bAutomatic_(true)
{
}

ConnectionSettings::ConnectionSettings(const ProtoTypes::ConnectionSettings &s)
    : protocol_(s.protocol()), port_(s.port()), bAutomatic_(s.is_automatic())
{
}

void ConnectionSettings::set(const ProtocolType &protocol, uint port, bool bAutomatic)
{
    protocol_ = protocol;
    port_ = port;
    bAutomatic_ = bAutomatic;
}

void ConnectionSettings::set(const ProtocolType &protocol, uint port)
{
    protocol_ = protocol;
    port_ = port;
}

void ConnectionSettings::set(const ConnectionSettings &s)
{
    protocol_ = s.protocol_;
    port_ = s.port_;
    bAutomatic_ = s.bAutomatic_;
}

void ConnectionSettings::setPort(uint port)
{
    port_ = port;
}

ProtocolType ConnectionSettings::protocol() const
{
    Q_ASSERT(protocol_.isInitialized());
    return protocol_;
}

void ConnectionSettings::setProtocol(ProtocolType protocol)
{
    protocol_ = protocol;
}

uint ConnectionSettings::port() const
{
    return port_;
}

bool ConnectionSettings::isAutomatic() const
{
    return bAutomatic_;
}

void ConnectionSettings::setIsAutomatic(bool isAutomatic)
{
    bAutomatic_ = isAutomatic;
}

bool ConnectionSettings::readFromSettingsV1(QSettings &settings)
{
    if (settings.contains("connectionProtocol") && settings.contains("connectionPort"))
    {
        protocol_ = ProtocolType(settings.value("connectionProtocol", "").toString());
        port_ = settings.value("connectionPort", 0).toUInt();
        bAutomatic_ = settings.value("connectionAutomatic", true).toBool();
        return true;
    }
    else
    {
        return false;
    }
}
void ConnectionSettings::debugToLog() const
{
    qCDebug(LOG_BASIC) << "Connection settings automatic:" << bAutomatic_;
    qCDebug(LOG_BASIC) << "Connection settings protocol:" << protocol_.toLongString();
    qCDebug(LOG_BASIC) << "Connection settings port:" << port_;
}

void ConnectionSettings::logConnectionSettings() const
{
    if (bAutomatic_)
    {
        qCDebug(LOG_CONNECTION) << "Connection settings: automatic";
    }
    else
    {
        qCDebug(LOG_CONNECTION) << "Connection settings: manual " << protocol_.toLongString() << port_;
    }
}

ProtoTypes::ConnectionSettings ConnectionSettings::convertToProtobuf() const
{
    ProtoTypes::ConnectionSettings cs;
    cs.set_protocol(protocol_.convertToProtobuf());
    cs.set_port(port_);
    cs.set_is_automatic(bAutomatic_);
    return cs;
}

} //namespace types

