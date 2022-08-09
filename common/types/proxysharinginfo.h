#ifndef TYPES_PROXYSHARINGINFO_H
#define TYPES_PROXYSHARINGINFO_H

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

    /*friend QDataStream& operator <<(QDataStream &stream, const ProxySharingInfo &o)
    {
        stream << versionForSerialization_;
        stream << o.mode << o.when;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream &stream, ProxySharingInfo &o)
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

#endif // TYPES_PROXYSHARINGINFO_H
