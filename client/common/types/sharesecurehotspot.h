#pragma once

#include "types/enums.h"

#include <QDataStream>
#include <QDebug>


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
        stream << o.isEnabled << o.password << o.ssid;
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

        if (version >= 2) {
            stream >> o.ssid;
        }
        return stream;
    }

    friend QDebug operator<<(QDebug dbg, const ShareSecureHotspot &s)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace();
        dbg << "{isEnabled:" << s.isEnabled << "; ";
        dbg << "ssid:" << (s.ssid.isEmpty() ? "empty" : "settled") << "; ";
        dbg << "password:" << (s.password.isEmpty() ? "empty" : "settled") << "}";
        return dbg;
    }


private:
    static constexpr quint32 versionForSerialization_ = 2;  // should increment the version if the data format is changed

};

} // types namespace
