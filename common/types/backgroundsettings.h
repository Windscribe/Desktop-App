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
};


} // types namespace

#endif // TYPES_BACKGROUNDSETTINGS_H
