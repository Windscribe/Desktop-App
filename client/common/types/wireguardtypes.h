#pragma once

#include <QtCore>

namespace types {

enum class WireGuardState
{
    NONE,       // WireGuard is not started.
    FAILURE,    // WireGuard is in error state.
    STARTING,   // WireGuard is initializing/starting up.
    LISTENING,  // WireGuard is listening for UAPI commands, but not connected.
    CONNECTING, // WireGuard is configured and awaits for a handshake.
    ACTIVE,     // WireGuard is connected.
};

struct WireGuardStatus
{
    WireGuardState state;
    quint32 errorCode;
    quint64 lastHandshake; /* On Windows: time of the last handshake, in 100ns intervals since 1601-01-01 UTC */
    quint64 bytesTransmitted;
    quint64 bytesReceived;
};

} //namespace types
