#ifndef TYPES_SHARESECUREHOTSPOT_H
#define TYPES_SHARESECUREHOTSPOT_H

#include "types/enums.h"

#include <QDataStream>


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

    friend QDataStream& operator <<(QDataStream &stream, const ShareSecureHotspot &o)
    {
        stream << versionForSerialization_;
        stream << o.isEnabled << o.password;
        return stream;
    }

    friend QDataStream& operator >>(QDataStream &stream, ShareSecureHotspot &o)
    {
        quint32 version;
        stream >> version;
        if (version > o.versionForSerialization_)
        {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> o.isEnabled >> o.password;
        return stream;
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed

};


} // types namespace

#endif // TYPES_SHARESECUREHOTSPOT_H
