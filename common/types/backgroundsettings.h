#ifndef TYPES_BACKGROUNDSETTINGS_H
#define TYPES_BACKGROUNDSETTINGS_H

#include "types/enums.h"

namespace types {

struct BackgroundSettings
{
    BACKGROUND_TYPE backgroundType = BACKGROUND_TYPE_COUNTRY_FLAGS;
    QString backgroundImageDisconnected;
    QString backgroundImageConnected;

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

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};


} // types namespace

#endif // TYPES_BACKGROUNDSETTINGS_H
