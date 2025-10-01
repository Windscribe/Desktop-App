#pragma once

#include "types/enums.h"

#include <filesystem>
#include <QDebug>
#include <QJsonObject>

namespace types {

struct SoundSettings
{
    SOUND_NOTIFICATION_TYPE disconnectedSoundType = SOUND_NOTIFICATION_TYPE_NONE;
    QString disconnectedSoundPath;
    SOUND_NOTIFICATION_TYPE connectedSoundType = SOUND_NOTIFICATION_TYPE_NONE;
    QString connectedSoundPath;

    SoundSettings() = default;

    SoundSettings(const QJsonObject &json)
    {
        if (json.contains(kJsonDisconnectedSoundTypeProp) && json[kJsonDisconnectedSoundTypeProp].isDouble()) {
            disconnectedSoundType = static_cast<SOUND_NOTIFICATION_TYPE>(json[kJsonDisconnectedSoundTypeProp].toInt());
        }

        if (json.contains(kJsonSoundDisconnectedProp) && json[kJsonSoundDisconnectedProp].isString()) {
            disconnectedSoundPath = json[kJsonSoundDisconnectedProp].toString();
        }

        if (json.contains(kJsonConnectedSoundTypeProp) && json[kJsonConnectedSoundTypeProp].isDouble()) {
            connectedSoundType = static_cast<SOUND_NOTIFICATION_TYPE>(json[kJsonConnectedSoundTypeProp].toInt());
        }

        if (json.contains(kJsonSoundConnectedProp) && json[kJsonSoundConnectedProp].isString()) {
            connectedSoundPath = json[kJsonSoundConnectedProp].toString();
        }
    }

    bool operator==(const SoundSettings &other) const
    {
        return other.disconnectedSoundType == disconnectedSoundType &&
               other.disconnectedSoundPath == disconnectedSoundPath &&
               other.connectedSoundType == connectedSoundType &&
               other.connectedSoundPath == connectedSoundPath;
    }

    bool operator!=(const SoundSettings &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson(bool isForDebugLog) const
    {
        QJsonObject json;
        if (!isForDebugLog) {
            json[kJsonSoundDisconnectedProp] = disconnectedSoundPath;
            json[kJsonSoundConnectedProp] = connectedSoundPath;
        }
        json[kJsonDisconnectedSoundTypeProp] = static_cast<int>(disconnectedSoundType);
        json[kJsonConnectedSoundTypeProp] = static_cast<int>(connectedSoundType);
        return json;
    }

    friend QDataStream& operator <<(QDataStream &stream, const SoundSettings &o)
    {
        stream << versionForSerialization_;
        stream << o.disconnectedSoundType << o.disconnectedSoundPath << o.connectedSoundType << o.connectedSoundPath;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, SoundSettings &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_)
        {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.disconnectedSoundType >> o.disconnectedSoundPath >> o.connectedSoundType >> o.connectedSoundPath;
        return stream;
    }

    friend QDebug operator<<(QDebug dbg, const SoundSettings &s)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{disconnectedSoundType:" << s.disconnectedSoundType << "; ";
        dbg << "connectedSoundType:" << s.connectedSoundType << "; ";
        dbg << "disconnectedSoundPath:" << (s.disconnectedSoundPath.isEmpty() ? "empty" : "set") << "; ";
        dbg << "connectedSoundPath:" << (s.connectedSoundPath.isEmpty() ? "empty" : "set") << "; ";
        dbg << "}";
        return dbg;
    }

private:
    static const inline QString kJsonDisconnectedSoundTypeProp = "disconnectedSoundType";
    static const inline QString kJsonSoundDisconnectedProp = "disconnectedSoundPath";
    static const inline QString kJsonConnectedSoundTypeProp = "connectedSoundType";
    static const inline QString kJsonSoundConnectedProp = "connectedSoundPath";

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} // types namespace
