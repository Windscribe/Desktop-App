#pragma once

#include "types/enums.h"

#include <QJsonObject>
#include <QSettings>

namespace types {

struct PacketSize
{
    PacketSize() {}

    PacketSize(const QJsonObject &json)
    {
        if (json.contains(kJsonIsAutomaticProp) && json[kJsonIsAutomaticProp].isBool())
            isAutomatic = json[kJsonIsAutomaticProp].toBool();

        if (json.contains(kJsonMTUProp) && json[kJsonMTUProp].isDouble()) {
            int mtuValue = json[kJsonMTUProp].toInt();
            // MTU is unset, or must be a minimum of 68 per RFC 791 and can't exceed maximum IP packet size of 65535
            if (mtuValue == -1 || (mtuValue >= 68 && mtuValue < 65536)) {
                mtu = mtuValue;
            }
        }
    }

    bool isAutomatic = true;
    int mtu = -1;

    bool operator==(const PacketSize &other) const
    {
        return other.isAutomatic == isAutomatic && other.mtu == mtu;
    }

    bool operator!=(const PacketSize &other) const
    {
        return !(*this == other);
    }

    void fromIni(const QSettings &settings)
    {
        QString prevMode = TOGGLE_MODE_toString(isAutomatic ? TOGGLE_MODE_AUTO : TOGGLE_MODE_MANUAL);
        TOGGLE_MODE mode = TOGGLE_MODE_fromString(settings.value(kJsonIsAutomaticProp, prevMode).toString());
        isAutomatic = (mode == TOGGLE_MODE_AUTO);
        int mtuValue = settings.value(kJsonMTUProp, mtu).toInt();
        // MTU is unset, or must be a minimum of 68 per RFC 791 and can't exceed maximum IP packet size of 65535
        if (mtuValue == -1 || (mtuValue >= 68 && mtuValue < 65536)) {
            mtu = mtuValue;
        }
    }

    void toIni(QSettings &settings) const
    {
        settings.setValue(kIniIsAutomaticProp, TOGGLE_MODE_toString(isAutomatic ? TOGGLE_MODE_AUTO : TOGGLE_MODE_MANUAL));
        settings.setValue(kIniMTUProp, mtu);
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[kJsonIsAutomaticProp] = isAutomatic;
        json[kJsonMTUProp] = mtu;
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

private:
    static const inline QString kIniIsAutomaticProp = "PacketSizeMode";
    static const inline QString kIniMTUProp = "PacketSizeMTU";

    static const inline QString kJsonIsAutomaticProp = "isAutomatic";
    static const inline QString kJsonMTUProp = "mtu";

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} // types namespace
