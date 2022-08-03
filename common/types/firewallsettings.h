#ifndef TYPES_FIREWALLSETTINGS_H
#define TYPES_FIREWALLSETTINGS_H

#include "types/enums.h"


namespace types {

struct FirewallSettings
{
    FirewallSettings() :
        mode(FIREWALL_MODE_AUTOMATIC),
        when(FIREWALL_WHEN_BEFORE_CONNECTION)
    {}

    FIREWALL_MODE mode;
    FIREWALL_WHEN when;

    bool operator==(const FirewallSettings &other) const
    {
        return other.mode == mode &&
               other.when == when;
    }

    bool operator!=(const FirewallSettings &other) const
    {
        return !(*this == other);
    }

    friend QDataStream& operator <<(QDataStream &stream, const FirewallSettings &o)
    {
        stream << versionForSerialization_;
        stream << o.mode << o.when;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream &stream, FirewallSettings &o)
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
    }

private:
    static constexpr quint32 versionForSerialization_ = 1;
};


} // types namespace

#endif // TYPES_CHECKUPDATE_H
