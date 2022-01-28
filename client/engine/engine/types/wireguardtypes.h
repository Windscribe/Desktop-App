#ifndef WIREGUARDTYPES_H
#define WIREGUARDTYPES_H

#include <QtCore>

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
    quint64 bytesReceived;
    quint64 bytesTransmitted;
};

#endif // WIREGUARDTYPES_H
