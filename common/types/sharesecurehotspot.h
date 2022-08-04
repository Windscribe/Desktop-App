#ifndef TYPES_SHARESECUREHOTSPOT_H
#define TYPES_SHARESECUREHOTSPOT_H

#include "types/enums.h"


namespace types {

struct ShareSecureHotspot
{
    bool isEnabled = false;
    QString ssid;
    QString password;

    bool operator==(const ShareSecureHotspot &other) const
    {
        return other.isEnabled == isEnabled &&
               other.ssid == ssid &&
               other.password == password;
    }

    bool operator!=(const ShareSecureHotspot &other) const
    {
        return !(*this == other);
    }
};


} // types namespace

#endif // TYPES_SHARESECUREHOTSPOT_H
