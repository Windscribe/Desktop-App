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

    friend QDataStream& operator <<(QDataStream &stream, const PacketSize &o)
    {
        stream << versionForSerialization_;
        stream << o.isAutomatic << o.mtu;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream &stream, PacketSize &o)
    {
        quint32 version;
        stream >> version;
        Q_ASSERT(version == versionForSerialization_);
        if (version > versionForSerialization_)
        {
            return stream;
        }
        stream >> o.isAutomatic >> o.mtu;
        return stream;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;

};


} // types namespace

#endif // TYPES_PACKETSIZE_H
