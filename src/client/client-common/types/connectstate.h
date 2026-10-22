#pragma once

#include "types/enums.h"
#include "types/locationid.h"

namespace types {

struct ConnectState
{
    CONNECT_STATE connectState = CONNECT_STATE_DISCONNECTED;
    DISCONNECT_REASON disconnectReason = DISCONNECTED_BY_USER;
    ConnectError connectError = ConnectError::kNoError;
    // Daemon fatal text for connectError. Empty for every other error. Not a persisted field: it
    // rides the in-process state signal and the CLI IPC state, both written and read by one install.
    QString connectErrorDetail;
    LocationID location;

    bool operator==(const ConnectState &other) const
    {
        return other.connectState == connectState &&
               other.disconnectReason == disconnectReason &&
               other.connectError == connectError &&
               other.location == location &&
               other.connectErrorDetail == connectErrorDetail;
    }

    bool operator!=(const ConnectState &other) const
    {
        return !(*this == other);
    }

    friend QDataStream& operator <<(QDataStream &stream, const ConnectState &o)
    {
        stream << versionForSerialization_;
        stream << o.connectState << o.disconnectReason << o.connectError << o.location << o.connectErrorDetail;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, ConnectState &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_)
        {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.connectState >> o.disconnectReason >> o.connectError >> o.location >> o.connectErrorDetail;
        return stream;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} // types namespace
