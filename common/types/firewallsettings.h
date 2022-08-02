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

};


} // types namespace

#endif // TYPES_CHECKUPDATE_H
