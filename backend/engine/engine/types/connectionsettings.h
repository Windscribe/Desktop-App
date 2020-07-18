#ifndef CONNECTIONSETTINGS_H
#define CONNECTIONSETTINGS_H

#include <QSettings>
#include <QString>
#include "engine/types/protocoltype.h"
#include "ipc/generated_proto/types.pb.h"

struct ConnectionSettings
{
    ConnectionSettings();
    ConnectionSettings(const ProtoTypes::ConnectionSettings &s);
    void set(const ProtocolType &protocol, uint port, bool bAutomatic);
    void set(const ConnectionSettings &s);

    ProtocolType protocol() const;
    uint port() const;
    bool isAutomatic() const;

    bool readFromSettings(QSettings &settings);
    void writeToSettings(QSettings &settings);

    bool isInitialized() const;

    bool isEqual(const ConnectionSettings &s) const;

    void debugToLog() const;
    void logConnectionSettings();

    ProtoTypes::ConnectionSettings convertToProtobuf() const;

private:
    ProtocolType protocol_;
    uint    port_;
    bool    bAutomatic_;
    bool    bInitialized_;
};

#endif // CONNECTIONSETTINGS_H
