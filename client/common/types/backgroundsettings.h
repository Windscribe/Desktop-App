#pragma once

#include "types/enums.h"

#include <filesystem>
#include <QDebug>
#include <QJsonObject>

namespace types {

struct BackgroundSettings
{
    QString backgroundImageDisconnected;
    QString backgroundImageConnected;

    // Added in version 2
    ASPECT_RATIO_MODE aspectRatioMode = ASPECT_RATIO_MODE_FILL;
    BACKGROUND_TYPE disconnectedBackgroundType = BACKGROUND_TYPE_COUNTRY_FLAGS;
    BACKGROUND_TYPE connectedBackgroundType = BACKGROUND_TYPE_COUNTRY_FLAGS;

    BackgroundSettings() = default;

    BackgroundSettings(const QJsonObject &json)
    {
        // For backward compatibility, if JSON contains backgroundType, use it for both disconnectedBackgroundType and connectedBackgroundType
        if (json.contains(kJsonBackgroundTypeProp) && json[kJsonBackgroundTypeProp].isDouble()) {
            disconnectedBackgroundType = BACKGROUND_TYPE_fromInt(json[kJsonBackgroundTypeProp].toInt());
            connectedBackgroundType = BACKGROUND_TYPE_fromInt(json[kJsonBackgroundTypeProp].toInt());
        } else {
            // Otherwise, use the new values
            if (json.contains(kJsonDisconnectedBackgroundTypeProp) && json[kJsonDisconnectedBackgroundTypeProp].isDouble()) {
                disconnectedBackgroundType = BACKGROUND_TYPE_fromInt(json[kJsonDisconnectedBackgroundTypeProp].toInt());
            }
            if (json.contains(kJsonConnectedBackgroundTypeProp) && json[kJsonConnectedBackgroundTypeProp].isDouble()) {
                connectedBackgroundType = BACKGROUND_TYPE_fromInt(json[kJsonConnectedBackgroundTypeProp].toInt());
            }
        }

        if (json.contains(kJsonAspectRatioModeProp) && json[kJsonAspectRatioModeProp].isDouble()) {
            aspectRatioMode = ASPECT_RATIO_MODE_fromInt(json[kJsonAspectRatioModeProp].toInt());
        }

        if (json.contains(kJsonBackgroundImageDisconnectedProp) && json[kJsonBackgroundImageDisconnectedProp].isString()) {
            QString img = json[kJsonBackgroundImageDisconnectedProp].toString();
            backgroundImageDisconnected = img;
        }

        if (json.contains(kJsonBackgroundImageConnectedProp) && json[kJsonBackgroundImageConnectedProp].isString()) {
            QString img = json[kJsonBackgroundImageConnectedProp].toString();
            backgroundImageConnected = img;
        }
    }

    bool operator==(const BackgroundSettings &other) const
    {
        return other.disconnectedBackgroundType == disconnectedBackgroundType &&
               other.connectedBackgroundType == connectedBackgroundType &&
               other.backgroundImageDisconnected == backgroundImageDisconnected &&
               other.backgroundImageConnected == backgroundImageConnected &&
               other.aspectRatioMode == aspectRatioMode;
    }

    bool operator!=(const BackgroundSettings &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson(bool isForDebugLog) const
    {
        QJsonObject json;
        if (!isForDebugLog) {
            json[kJsonBackgroundImageDisconnectedProp] = backgroundImageDisconnected;
            json[kJsonBackgroundImageConnectedProp] = backgroundImageConnected;
        }
        json[kJsonAspectRatioModeProp] = static_cast<int>(aspectRatioMode);
        json[kJsonDisconnectedBackgroundTypeProp] = static_cast<int>(disconnectedBackgroundType);
        json[kJsonConnectedBackgroundTypeProp] = static_cast<int>(connectedBackgroundType);
        return json;
    }

    friend QDataStream& operator <<(QDataStream &stream, const BackgroundSettings &o)
    {
        stream << versionForSerialization_;
        stream << o.disconnectedBackgroundType << o.connectedBackgroundType << o.backgroundImageDisconnected << o.backgroundImageConnected << o.aspectRatioMode;
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

        if (version == 1) {
            // For backward compatibility, if version is 1, use backgroundType for both disconnectedBackgroundType and connectedBackgroundType
            BACKGROUND_TYPE backgroundType;
            stream >> backgroundType;
            o.disconnectedBackgroundType = backgroundType;
            o.connectedBackgroundType = backgroundType;
            stream >> o.backgroundImageDisconnected >> o.backgroundImageConnected;
        } else {
            stream >> o.disconnectedBackgroundType >> o.connectedBackgroundType >> o.backgroundImageDisconnected >> o.backgroundImageConnected >> o.aspectRatioMode;
        }

        return stream;
    }

private:
    static const inline QString kJsonBackgroundTypeProp = "backgroundType";
    static const inline QString kJsonDisconnectedBackgroundTypeProp = "disconnectedBackgroundType";
    static const inline QString kJsonConnectedBackgroundTypeProp = "connectedBackgroundType";
    static const inline QString kJsonBackgroundImageDisconnectedProp = "backgroundImageDisconnected";
    static const inline QString kJsonBackgroundImageConnectedProp = "backgroundImageConnected";
    static const inline QString kJsonAspectRatioModeProp = "aspectRatioMode";

    static constexpr quint32 versionForSerialization_ = 2;  // should increment the version if the data format is changed
};

} // types namespace
