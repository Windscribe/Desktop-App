#ifndef TYPES_LOCATIONITEM_H
#define TYPES_LOCATIONITEM_H

#include "enums.h"
#include "locationid.h"
#include "pingtime.h"

namespace types {

struct CityItem
{
    LocationID id;
    QString city;
    QString nick;
    PingTime pingTimeMs;
    bool isPro = false;
    bool isDisabled = false;

    // specific for static IP location
    QString staticIpCountryCode;
    QString staticIpType;
    QString staticIp;

    // specific for custom config location
    CUSTOM_CONFIG_TYPE customConfigType = CUSTOM_CONFIG_OPENVPN;
    bool customConfigIsCorrect = false;
    QString customConfigErrorMessage;

    int link_speed = 100;
    int health = 0;
};

struct LocationItem
{
    LocationID id;
    QString name;
    QString countryCode;
    bool isPremiumOnly = false;
    int p2p = 0;
    QVector<CityItem> cities;
};


} //namespace types

#endif // TYPES_LOCATIONITEM_H
