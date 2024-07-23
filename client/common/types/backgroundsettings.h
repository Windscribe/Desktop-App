#pragma once

#include "types/enums.h"

#include <filesystem>
#include <QDebug>
#include <QJsonObject>

namespace types {

struct BackgroundSettings
{
    BACKGROUND_TYPE backgroundType = BACKGROUND_TYPE_COUNTRY_FLAGS;
    QString backgroundImageDisconnected;
    QString backgroundImageConnected;

    BackgroundSettings() = default;

    BackgroundSettings(const QJsonObject &json)
    {
        if (json.contains(kJsonBackgroundTypeProp) && json[kJsonBackgroundTypeProp].isDouble()) {
            backgroundType = BACKGROUND_TYPE_fromInt(json[kJsonBackgroundTypeProp].toInt());
        }

        if (json.contains(kJsonBackgroundImageDisconnectedProp) && json[kJsonBackgroundImageDisconnectedProp].isString()) {
            QString img = json[kJsonBackgroundImageDisconnectedProp].toString();
            std::error_code ec;
            if (std::filesystem::exists(img.toStdString(), ec)) {
                backgroundImageDisconnected = img;
            }
        }

        if (json.contains(kJsonBackgroundImageConnectedProp) && json[kJsonBackgroundImageConnectedProp].isString()) {
            QString img = json[kJsonBackgroundImageConnectedProp].toString();
            std::error_code ec;
            if (std::filesystem::exists(img.toStdString(), ec)) {
                backgroundImageConnected = img;
            }
        }
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
        json[kJsonBackgroundTypeProp] = static_cast<int>(backgroundType);
        json[kJsonBackgroundImageDisconnectedProp] = backgroundImageDisconnected;
        json[kJsonBackgroundImageConnectedProp] = backgroundImageConnected;
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
    static const inline QString kJsonBackgroundTypeProp = "backgroundType";
    static const inline QString kJsonBackgroundImageDisconnectedProp = "backgroundImageDisconnected";
    static const inline QString kJsonBackgroundImageConnectedProp = "backgroundImageConnected";

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} // types namespace
