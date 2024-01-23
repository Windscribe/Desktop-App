#pragma once

#include "types/enums.h"

#include <QDebug>
#include <QJsonObject>

namespace types {

struct ShareProxyGateway
{
    struct JsonInfo
    {
        JsonInfo& operator=(const JsonInfo&) { return *this; }

        const QString kIsEnabledProp = "isEnabled";
        const QString kProxySharingModeProp = "proxySharingMode";
    };

    ShareProxyGateway() = default;

    ShareProxyGateway(const QJsonObject &json)
    {
        if (json.contains(jsonInfo.kIsEnabledProp) && json[jsonInfo.kIsEnabledProp].isBool())
            isEnabled = json[jsonInfo.kIsEnabledProp].toBool();

        if (json.contains(jsonInfo.kProxySharingModeProp) && json[jsonInfo.kProxySharingModeProp].isDouble())
            proxySharingMode = static_cast<PROXY_SHARING_TYPE>(json[jsonInfo.kProxySharingModeProp].toInt());
    }

    bool isEnabled = false;
    PROXY_SHARING_TYPE proxySharingMode = PROXY_SHARING_HTTP;
    JsonInfo jsonInfo;

    bool operator==(const ShareProxyGateway &other) const
    {
        return other.isEnabled == isEnabled &&
               other.proxySharingMode == proxySharingMode;
    }

    bool operator!=(const ShareProxyGateway &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[jsonInfo.kIsEnabledProp] = isEnabled;
        json[jsonInfo.kProxySharingModeProp] = static_cast<int>(proxySharingMode);
        return json;
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
