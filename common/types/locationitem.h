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

    int linkSpeed = 100;
    int health = 0;

    bool operator==(const CityItem &other) const;
    bool operator!=(const CityItem &other) const;
};

struct LocationItem
{
    LocationID id;
    QString name;
    QString countryCode;
    bool isPremiumOnly = false;
    bool isNoP2P = false;
    QVector<CityItem> cities;

    bool operator==(const LocationItem &other) const;
    bool operator!=(const LocationItem &other) const;

    // utils functions
    static QVector<LocationItem> loadLocationsFromJson(const QByteArray &arr);
    static LocationItem loadLocationFromJson(const QByteArray &arr);

private:
    static LocationItem locationFromJsonObject(const QJsonObject &obj);

};




} //namespace types

#endif // TYPES_LOCATIONITEM_H
