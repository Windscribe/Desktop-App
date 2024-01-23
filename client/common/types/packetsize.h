#pragma once

#include "types/enums.h"

#include <QJsonObject>

namespace types {

struct PacketSize
{
    struct JsonInfo
    {
        JsonInfo& operator=(const JsonInfo&) { return *this; }

        const QString kIsAutomaticProp = "isAutomatic";
        const QString kMTUProp = "mtu";
    };

    PacketSize() :
        isAutomatic(true),
        mtu(-1)     // -1 not set
    {}

    PacketSize(const QJsonObject &json)
    {
        if (json.contains(jsonInfo.kIsAutomaticProp) && json[jsonInfo.kIsAutomaticProp].isBool())
            isAutomatic = json[jsonInfo.kIsAutomaticProp].toBool();

        if (json.contains(jsonInfo.kMTUProp) && json[jsonInfo.kMTUProp].isDouble())
            mtu = json[jsonInfo.kMTUProp].toInt();
    }

    bool isAutomatic;
    int mtu;
    JsonInfo jsonInfo;

    bool operator==(const PacketSize &other) const
    {
        return other.isAutomatic == isAutomatic && other.mtu == mtu;
    }

    bool operator!=(const PacketSize &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[jsonInfo.kIsAutomaticProp] = isAutomatic;
        json[jsonInfo.kMTUProp] = mtu;
        return json;
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
        if (version > o.versionForSerialization_)
        {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.isAutomatic >> o.mtu;
        return stream;
    }


    friend QDebug operator<<(QDebug dbg, const PacketSize &ps)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{isAutomatic:" << ps.isAutomatic << "; ";
        dbg << "mtu:" << ps.mtu << "}";
        return dbg;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} // types namespace
