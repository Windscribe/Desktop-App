#ifndef TYPES_PACKETSIZE_H
#define TYPES_PACKETSIZE_H

#include "types/enums.h"

#include <QJsonObject>


namespace types {

struct PacketSize
{
    PacketSize() :
        isAutomatic(true),
        mtu(-1)     // -1 not set
    {}

    bool isAutomatic;
    int mtu;

    bool operator==(const PacketSize &other) const
    {
        return other.isAutomatic == isAutomatic && other.mtu == mtu;
    }

    bool operator!=(const PacketSize &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJsonObject() const
    {
        QJsonObject json;
        json["isAutomatic"] = isAutomatic;
        json["mtu"] = mtu;
        return json;
    }

    bool fromJsonObject(const QJsonObject &json)
    {
        if (json.contains("isAutomatic")) isAutomatic = json["isAutomatic"].toInt(true);
        if (json.contains("mtu")) mtu = json["mtu"].toInt(-1);
        return true;
    }

    friend QDebug operator<<(QDebug dbg, const PacketSize &ps)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{isAutomatic:" << ps.isAutomatic << "; ";
        dbg << "mtu:" << ps.mtu << "}";
        return dbg;
    }
};


} // types namespace

#endif // TYPES_PACKETSIZE_H
