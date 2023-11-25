#pragma once

#include "types/enums.h"
#include "types/locationid.h"

namespace types {

struct ConnectState
{
    CONNECT_STATE connectState = CONNECT_STATE_DISCONNECTED;
    DISCONNECT_REASON disconnectReason = DISCONNECTED_BY_USER;
    CONNECT_ERROR connectError = NO_CONNECT_ERROR;
    LocationID location;

    bool operator==(const ConnectState &other) const
    {
        return other.connectState == connectState &&
               other.disconnectReason == disconnectReason &&
               other.connectError == connectError &&
               other.location == location;
    }

    bool operator!=(const ConnectState &other) const
    {
        return !(*this == other);
    }

    friend QDataStream& operator <<(QDataStream &stream, const ConnectState &o)
    {
        stream << versionForSerialization_;
        stream << o.connectState << o.disconnectReason << o.connectError << o.location;
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
        stream >> o.connectState >> o.disconnectReason >> o.connectError >> o.location;
        return stream;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} // types namespace
