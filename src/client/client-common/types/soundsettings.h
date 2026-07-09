#pragma once

#include "types/enums.h"
#include "utils/log/categories.h"

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
            disconnectedSoundType = enumFromInt<SOUND_NOTIFICATION_TYPE>(json[kJsonDisconnectedSoundTypeProp].toInt());
        }

        if (json.contains(kJsonSoundDisconnectedProp) && json[kJsonSoundDisconnectedProp].isString()) {
            disconnectedSoundPath = json[kJsonSoundDisconnectedProp].toString();
        }

        if (json.contains(kJsonConnectedSoundTypeProp) && json[kJsonConnectedSoundTypeProp].isDouble()) {
            connectedSoundType = enumFromInt<SOUND_NOTIFICATION_TYPE>(json[kJsonConnectedSoundTypeProp].toInt());
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

    void validate()
    {
        disconnectedSoundType = enumFromInt<SOUND_NOTIFICATION_TYPE>(static_cast<int>(disconnectedSoundType));
        connectedSoundType = enumFromInt<SOUND_NOTIFICATION_TYPE>(static_cast<int>(connectedSoundType));
        validatePair(disconnectedSoundType, disconnectedSoundPath);
        validatePair(connectedSoundType, connectedSoundPath);
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
    static void validatePair(SOUND_NOTIFICATION_TYPE &type, QString &path)
    {
        if (type == SOUND_NOTIFICATION_TYPE_NONE) {
            path.clear();
            return;
        }
        if (type == SOUND_NOTIFICATION_TYPE_BUNDLED) {
            if (!path.startsWith(":/sounds/")) {
                qCWarning(LOG_BASIC) << "SoundSettings: bundled sound path is not a sound resource, resetting";
                type = SOUND_NOTIFICATION_TYPE_NONE;
                path.clear();
            }
            return;
        }
        // SOUND_NOTIFICATION_TYPE_CUSTOM
        constexpr int kMaxPathLen = 4096;
        if (path.size() > kMaxPathLen || path.contains(QChar(0))) {
            qCWarning(LOG_BASIC) << "SoundSettings: custom sound path out of bounds, resetting";
            type = SOUND_NOTIFICATION_TYPE_NONE;
            path.clear();
            return;
        }
        std::error_code ec;
        std::filesystem::path p(path.toStdString());
        if (!p.is_absolute() || !std::filesystem::is_regular_file(p, ec)) {
            qCWarning(LOG_BASIC) << "SoundSettings: custom sound path is not a valid absolute file, resetting";
            type = SOUND_NOTIFICATION_TYPE_NONE;
            path.clear();
        }
    }

    static const inline QString kJsonDisconnectedSoundTypeProp = "disconnectedSoundType";
    static const inline QString kJsonSoundDisconnectedProp = "disconnectedSoundPath";
    static const inline QString kJsonConnectedSoundTypeProp = "connectedSoundType";
    static const inline QString kJsonSoundConnectedProp = "connectedSoundPath";

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} // types namespace
