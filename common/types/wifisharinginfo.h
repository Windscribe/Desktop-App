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

    /*friend QDataStream& operator <<(QDataStream &stream, const WifiSharingInfo &o)
    {
        stream << versionForSerialization_;
        stream << o.mode << o.when;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream &stream, WifiSharingInfo &o)
    {
        quint32 version;
        stream >> version;
        Q_ASSERT(version == versionForSerialization_);
        if (version > versionForSerialization_)
        {
            return stream;
        }
        stream >> o.mode >> o.when;
        return stream;
    }*/

private:
    //static constexpr quint32 versionForSerialization_ = 1;
};


} // types namespace

#endif // TYPES_CHECKUPDATE_H
