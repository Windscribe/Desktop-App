#pragma once

#include "types/enums.h"

#include <QDebug>
#include <QJsonObject>

namespace types {

struct BackgroundSettings
{
    struct JsonInfo
    {
        JsonInfo& operator=(const JsonInfo&) { return *this; }

        const QString kBackgroundTypeProp = "backgroundType";
        const QString kBackgroundImageDisconnectedProp = "backgroundImageDisconnected";
        const QString kBackgroundImageConnectedProp = "backgroundImageConnected";
    };

    BACKGROUND_TYPE backgroundType = BACKGROUND_TYPE_COUNTRY_FLAGS;
    QString backgroundImageDisconnected;
    QString backgroundImageConnected;
    JsonInfo jsonInfo;

    BackgroundSettings() = default;

    BackgroundSettings(const QJsonObject &json)
    {
        if (json.contains(jsonInfo.kBackgroundTypeProp) && json[jsonInfo.kBackgroundTypeProp].isDouble())
            backgroundType = static_cast<BACKGROUND_TYPE>(json[jsonInfo.kBackgroundTypeProp].toInt());

        if (json.contains(jsonInfo.kBackgroundImageDisconnectedProp) && json[jsonInfo.kBackgroundImageDisconnectedProp].isString())
            backgroundImageDisconnected = json[jsonInfo.kBackgroundImageDisconnectedProp].toString();

        if (json.contains(jsonInfo.kBackgroundImageConnectedProp) && json[jsonInfo.kBackgroundImageConnectedProp].isString())
            backgroundImageConnected = json[jsonInfo.kBackgroundImageConnectedProp].toString();
    }

    bool operator==(const BackgroundSettings &other) const
    {
        return other.backgroundType == backgroundType &&
               other.backgroundImageDisconnected == backgroundImageDisconnected &&
               other.backgroundImageConnected == backgroundImageConnected;
    }

    bool operator!=(const BackgroundSettings &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[jsonInfo.kBackgroundTypeProp] = static_cast<int>(backgroundType);
        json[jsonInfo.kBackgroundImageDisconnectedProp] = backgroundImageDisconnected;
        json[jsonInfo.kBackgroundImageConnectedProp] = backgroundImageConnected;
        return json;
    }

    friend QDataStream& operator <<(QDataStream &stream, const BackgroundSettings &o)
    {
        stream << versionForSerialization_;
        stream << o.backgroundType << o.backgroundImageDisconnected << o.backgroundImageConnected;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, BackgroundSettings &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_)
        {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.backgroundType >> o.backgroundImageDisconnected >> o.backgroundImageConnected;
        return stream;
    }

    friend QDebug operator<<(QDebug dbg, const BackgroundSettings &s)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{backgroundType:" << s.backgroundType << "; ";
        dbg << "backgroundImageDisconnected:" << (s.backgroundImageDisconnected.isEmpty() ? "empty" : "settled") << "; ";
        dbg << "backgroundImageConnected:" << (s.backgroundImageConnected.isEmpty() ? "empty" : "settled") << "}";
        return dbg;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} // types namespace
