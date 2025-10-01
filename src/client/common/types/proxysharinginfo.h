#pragma once

#include "types/enums.h"

namespace types {

struct ProxySharingInfo
{
    ProxySharingInfo() :
        isEnabled(false),
        mode(PROXY_SHARING_HTTP),
        usersCount(0)
    {}

    bool isEnabled;
    PROXY_SHARING_TYPE mode;
    QString address;
    qint32 usersCount;


    bool operator==(const ProxySharingInfo &other) const
    {
        return other.isEnabled == isEnabled &&
               other.mode == mode &&
               other.address == address &&
               other.usersCount == usersCount;
    }

    bool operator!=(const ProxySharingInfo &other) const
    {
        return !(*this == other);
    }

};

} // types namespace
