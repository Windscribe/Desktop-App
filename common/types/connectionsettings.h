#ifndef TYPES_CONNECTIONSETTINGS_H
#define TYPES_CONNECTIONSETTINGS_H

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

    friend QDataStream& operator <<(QDataStream &stream, const ConnectionSettings &o)
    {
        stream << versionForSerialization_;
        stream << o.protocol_ << o.port_ << o.bAutomatic_;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream &stream, ConnectionSettings &o)
    {
        quint32 version;
        stream >> version;
        Q_ASSERT(version == versionForSerialization_);
        if (version > versionForSerialization_)
        {
            return stream;
        }
        stream >> o.protocol_ >> o.port_ >> o.bAutomatic_;
        return stream;
    }


private:
    ProtocolType protocol_;
    uint    port_;
    bool    bAutomatic_;

    static constexpr quint32 versionForSerialization_ = 1;
};

} //namespace types

#endif // TYPES_CONNECTIONSETTINGS_H
