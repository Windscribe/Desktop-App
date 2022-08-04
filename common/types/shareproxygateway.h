#ifndef TYPES_SHAREPROXYGATEWAY_H
#define TYPES_SHAREPROXYGATEWAY_H

#include "types/enums.h"

namespace types {

struct ShareProxyGateway
{
    bool isEnabled = false;
    PROXY_SHARING_TYPE proxySharingMode = PROXY_SHARING_HTTP;

    bool operator==(const ShareProxyGateway &other) const
    {
        return other.isEnabled == isEnabled &&
               other.proxySharingMode == proxySharingMode;
    }

    bool operator!=(const ShareProxyGateway &other) const
    {
        return !(*this == other);
    }
};


} // types namespace

#endif // TYPES_SHAREPROXYGATEWAY_H
