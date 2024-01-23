#pragma once

#include "types/enums.h"

#include <QJsonObject>

namespace types {

struct FirewallSettings
{
    struct JsonInfo
    {
        JsonInfo& operator=(const JsonInfo&) { return *this; }

        const QString kModeProp = "mode";
        const QString kWhenProp = "when";
    };

    FirewallSettings() :
        mode(FIREWALL_MODE_AUTOMATIC),
        when(FIREWALL_WHEN_BEFORE_CONNECTION)
    {}

    FirewallSettings(const QJsonObject &json)
    {
        if (json.contains(jsonInfo.kModeProp) && json[jsonInfo.kModeProp].isDouble())
            mode = static_cast<FIREWALL_MODE>(json[jsonInfo.kModeProp].toInt());

        if (json.contains(jsonInfo.kWhenProp) && json[jsonInfo.kWhenProp].isDouble())
            when = static_cast<FIREWALL_WHEN>(json[jsonInfo.kWhenProp].toInt());
    }

    FIREWALL_MODE mode;
    FIREWALL_WHEN when;
    JsonInfo jsonInfo;

    bool operator==(const FirewallSettings &other) const
    {
        return other.mode == mode &&
               other.when == when;
    }

    bool operator!=(const FirewallSettings &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[jsonInfo.kModeProp] = static_cast<int>(mode);
        json[jsonInfo.kWhenProp] = static_cast<int>(when);
        return json;
    }

    friend QDataStream& operator <<(QDataStream &stream, const FirewallSettings &o)
    {
        stream << versionForSerialization_;
        stream << o.mode << o.when;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, FirewallSettings &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_)
        {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.mode >> o.when;
        return stream;
    }


    friend QDebug operator<<(QDebug dbg, const FirewallSettings &fs)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{mode:" << FIREWALL_MODE_toString(fs.mode) << "; ";
        dbg << "when:" << FIREWALL_WHEN_toString(fs.when) << "}";
        return dbg;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed

};

} // types namespace
