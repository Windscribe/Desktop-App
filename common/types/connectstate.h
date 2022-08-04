#ifndef TYPES_CONNECTSTATE_H
#define TYPES_CONNECTSTATE_H

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
};


} // types namespace

#endif // TYPES_CONNECTSTATE_H
