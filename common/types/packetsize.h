#ifndef TYPES_PACKETSIZE_H
#define TYPES_PACKETSIZE_H

#include "types/enums.h"


namespace types {

struct PacketSize
{
    PacketSize() :
        isAutomatic(true),
        mtu(-1)     // -1 not set
    {}

    bool isAutomatic;
    qint32 mtu;

    bool operator==(const PacketSize &other) const
    {
        return other.isAutomatic == isAutomatic && other.mtu == mtu;
    }

    bool operator!=(const PacketSize &other) const
    {
        return !(*this == other);
    }

};


} // types namespace

#endif // TYPES_PACKETSIZE_H
