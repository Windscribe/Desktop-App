#ifndef TYPES_FIREWALLSETTINGS_H
#define TYPES_FIREWALLSETTINGS_H

#include "types/enums.h"

#include <QJsonObject>


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

    QJsonObject toJsonObject() const
    {
        QJsonObject json;
        json["mode"] = (int)mode;
        json["when"] = (int) when;
        return json;
    }

    bool fromJsonObject(const QJsonObject &json)
    {
        if (json.contains("mode")) mode = (FIREWALL_MODE)json["mode"].toInt(FIREWALL_MODE_AUTOMATIC);
        if (json.contains("when")) when = (FIREWALL_WHEN)json["when"].toInt(FIREWALL_WHEN_BEFORE_CONNECTION);
        return true;
    }

};


} // types namespace

#endif // TYPES_CHECKUPDATE_H
