#ifndef TYPES_WIFISHARINGINFO_H
#define TYPES_WIFISHARINGINFO_H

#include "types/enums.h"


namespace types {

struct WifiSharingInfo
{
    WifiSharingInfo() :
        isEnabled(false),
        usersCount(0)
    {}

    bool isEnabled;
    QString ssid;
    qint32 usersCount;

    bool operator==(const WifiSharingInfo &other) const
    {
        return other.isEnabled == isEnabled &&
               other.ssid == ssid &&
               other.usersCount == usersCount;
    }

    bool operator!=(const WifiSharingInfo &other) const
    {
        return !(*this == other);
    }

};


} // types namespace

#endif // TYPES_CHECKUPDATE_H
