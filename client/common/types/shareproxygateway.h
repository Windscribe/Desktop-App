#ifndef TYPES_SHAREPROXYGATEWAY_H
#define TYPES_SHAREPROXYGATEWAY_H

#include "types/enums.h"

#include <QDebug>

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

    friend QDataStream& operator <<(QDataStream &stream, const ShareProxyGateway &o)
    {
        stream << versionForSerialization_;
        stream << o.isEnabled << o.proxySharingMode;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, ShareProxyGateway &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_)
        {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.isEnabled >> o.proxySharingMode;
        return stream;
    }

    friend QDebug operator<<(QDebug dbg, const ShareProxyGateway &s)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{isEnabled:" << s.isEnabled << "; ";
        dbg << "proxySharingMode:" << PROXY_SHARING_TYPE_toString(s.proxySharingMode) << "}";
        return dbg;
    }


private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};


} // types namespace

#endif // TYPES_SHAREPROXYGATEWAY_H
