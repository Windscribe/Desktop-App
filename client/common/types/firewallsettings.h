#pragma once

#include "types/enums.h"

#include <QJsonObject>
#include <QSettings>

namespace types {

struct FirewallSettings
{
    FirewallSettings() {}

    FirewallSettings(const QJsonObject &json)
    {
        if (json.contains(kJsonModeProp) && json[kJsonModeProp].isDouble()) {
            mode = FIREWALL_MODE_fromInt(json[kJsonModeProp].toInt());
        }

        if (json.contains(kJsonWhenProp) && json[kJsonWhenProp].isDouble()) {
            when = FIREWALL_WHEN_fromInt(json[kJsonWhenProp].toInt());
        }
    }

    FIREWALL_MODE mode = FIREWALL_MODE_AUTOMATIC;
    FIREWALL_WHEN when = FIREWALL_WHEN_BEFORE_CONNECTION;

    bool isAlwaysOnMode() const
    {
        return mode == FIREWALL_MODE_ALWAYS_ON || mode == FIREWALL_MODE_ALWAYS_ON_PLUS;
    }

    bool operator==(const FirewallSettings &other) const
    {
        return other.mode == mode &&
               other.when == when;
    }

    bool operator!=(const FirewallSettings &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson(bool isForDebugLog) const
    {
        QJsonObject json;
        json[kJsonModeProp] = static_cast<int>(mode);
        json[kJsonWhenProp] = static_cast<int>(when);
        if (isForDebugLog) {
            json["modeDesc"] = FIREWALL_MODE_toString(mode);
            json["whenDesc"] = FIREWALL_WHEN_toString(when);
        }
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

private:
    static const inline QString kIniModeProp = "FirewallMode";
    static const inline QString kIniWhenProp = "FirewallWhen";

    static const inline QString kJsonModeProp = "mode";
    static const inline QString kJsonWhenProp = "when";

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} // types namespace
