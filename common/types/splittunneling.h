#ifndef TYPES_SPLITTUNNELING_H
#define TYPES_SPLITTUNNELING_H

#include "types/enums.h"


namespace types {


struct SplitTunnelingSettings
{
    bool active = false;
    SPLIT_TUNNELING_MODE mode = SPLIT_TUNNELING_MODE_EXCLUDE;

    bool operator==(const SplitTunnelingSettings &other) const
    {
        return other.active == active &&
               other.mode == mode;
    }

    bool operator!=(const SplitTunnelingSettings &other) const
    {
        return !(*this == other);
    }
};

struct SplitTunnelingNetworkRoute
{
    SPLIT_TUNNELING_NETWORK_ROUTE_TYPE type = SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP;
    QString name;

    bool operator==(const SplitTunnelingNetworkRoute &other) const
    {
        return other.type == type &&
               other.name == name;
    }

    bool operator!=(const SplitTunnelingNetworkRoute &other) const
    {
        return !(*this == other);
    }
};

struct SplitTunnelingApp
{
    SPLIT_TUNNELING_APP_TYPE type = SPLIT_TUNNELING_APP_TYPE_USER;
    QString name;
    QString fullName;   // path + name
    bool active = false;

    bool operator==(const SplitTunnelingApp &other) const
    {
        return other.type == type &&
               other.name == name &&
               other.fullName == fullName &&
               other.active == active;
    }

    bool operator!=(const SplitTunnelingApp &other) const
    {
        return !(*this == other);
    }
};


struct SplitTunneling
{
    SplitTunnelingSettings settings;
    QVector<SplitTunnelingApp> apps;
    QVector<SplitTunnelingNetworkRoute> networkRoutes;

    bool operator==(const SplitTunneling &other) const
    {
        return other.settings == settings &&
               other.apps == apps &&
               other.networkRoutes == networkRoutes;
    }

    bool operator!=(const SplitTunneling &other) const
    {
        return !(*this == other);
    }
};



} // types namespace

#endif // TYPES_SPLITTUNNELING_H
