#include "connectionsettings.h"
#include "Utils/logger.h"

const int typeIdConnectionSettings = qRegisterMetaType<ConnectionSettings>("ConnectionSettings");

ConnectionSettings::ConnectionSettings() : bInitialized_(false)
{

}

ConnectionSettings::ConnectionSettings(const ProtoTypes::ConnectionSettings &s)
{
    protocol_ = ProtocolType(s.protocol());
    port_ = s.port();
    bAutomatic_ = s.is_automatic();
    bInitialized_ = true;
}

void ConnectionSettings::set(const ProtocolType &protocol, uint port, bool bAutomatic)
{
    protocol_ = protocol;
    port_ = port;
    bAutomatic_ = bAutomatic;
    bInitialized_ = true;
}

void ConnectionSettings::set(const ConnectionSettings &s)
{
    protocol_ = s.protocol_;
    port_ = s.port_;
    bAutomatic_ = s.bAutomatic_;
    bInitialized_ = s.bInitialized_;
}

ProtocolType ConnectionSettings::protocol() const
{
    Q_ASSERT(protocol_.isInitialized());
    return protocol_;
}

uint ConnectionSettings::port() const
{
    Q_ASSERT(bInitialized_);
    return port_;
}

bool ConnectionSettings::isAutomatic() const
{
    Q_ASSERT(bInitialized_);
    return bAutomatic_;
}

bool ConnectionSettings::readFromSettings(QSettings &settings)
{
    if (settings.contains("connectionProtocol") && settings.contains("connectionPort"))
    {
        protocol_ = ProtocolType(settings.value("connectionProtocol", "").toString());
        port_ = settings.value("connectionPort", 0).toUInt();
        bAutomatic_ = settings.value("connectionAutomatic", true).toBool();
        bInitialized_ = true;
        return true;
    }
    else
    {
        return false;
    }
}

void ConnectionSettings::writeToSettings(QSettings &settings)
{
    Q_ASSERT(bInitialized_);
    settings.setValue("connectionProtocol", protocol_.toLongString());
    settings.setValue("connectionPort", port_);
    settings.setValue("connectionAutomatic", bAutomatic_);
}

bool ConnectionSettings::isInitialized() const
{
    return bInitialized_;
}

bool ConnectionSettings::isEqual(const ConnectionSettings &s) const
{
    Q_ASSERT(bInitialized_);
    Q_ASSERT(s.bInitialized_);
    return s.protocol_.isEqual(protocol_) && s.port_ == port_ &&
           s.bAutomatic_ == bAutomatic_ && s.bInitialized_ == bInitialized_;
}

void ConnectionSettings::debugToLog() const
{
    if (bInitialized_)
    {
        qCDebug(LOG_BASIC) << "Connection settings automatic:" << bAutomatic_;
        qCDebug(LOG_BASIC) << "Connection settings protocol:" << protocol_.toLongString();
        qCDebug(LOG_BASIC) << "Connection settings port:" << port_;
    }
}

void ConnectionSettings::logConnectionSettings()
{
    if (bInitialized_)
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
    else
    {
        qCDebug(LOG_CONNECTION) << "Connection settings not initialized!";
    }
}

ProtoTypes::ConnectionSettings ConnectionSettings::convertToProtobuf() const
{
    Q_ASSERT(bInitialized_);
    ProtoTypes::ConnectionSettings cs;
    cs.set_protocol(protocol_.convertToProtobuf());
    cs.set_port(port_);
    cs.set_is_automatic(bAutomatic_);
    return cs;
}


