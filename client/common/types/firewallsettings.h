#pragma once

#include "types/enums.h"

#include <QJsonObject>

namespace types {

struct FirewallSettings
{
    FirewallSettings() :
        mode(FIREWALL_MODE_AUTOMATIC),
        when(FIREWALL_WHEN_BEFORE_CONNECTION)
    {}

    FirewallSettings(const QJsonObject &json)
    {
        if (json.contains(kJsonModeProp) && json[kJsonModeProp].isDouble())
            mode = static_cast<FIREWALL_MODE>(json[kJsonModeProp].toInt());

        if (json.contains(kJsonWhenProp) && json[kJsonWhenProp].isDouble())
            when = static_cast<FIREWALL_WHEN>(json[kJsonWhenProp].toInt());
    }

    FirewallSettings(const QSettings &settings)
    {
        fromIni(settings);
    }

    FIREWALL_MODE mode;
    FIREWALL_WHEN when;

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
        json[kJsonModeProp] = static_cast<int>(mode);
        json[kJsonWhenProp] = static_cast<int>(when);
        return json;
    }

    void fromIni(const QSettings &settings)
    {
        mode = FIREWALL_MODE_fromString(settings.value(kIniModeProp, FIREWALL_MODE_toString(mode)).toString());
        when = FIREWALL_WHEN_fromString(settings.value(kIniWhenProp, FIREWALL_WHEN_toString(when)).toString());
    }

    void toIni(QSettings &settings) const
    {
        settings.setValue(kIniModeProp, FIREWALL_MODE_toString(mode));
        settings.setValue(kIniWhenProp, FIREWALL_WHEN_toString(when));
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
    static const inline QString kIniModeProp = "FirewallMode";
    static const inline QString kIniWhenProp = "FirewallWhen";

    static const inline QString kJsonModeProp = "mode";
    static const inline QString kJsonWhenProp = "when";

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} // types namespace
