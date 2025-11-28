#pragma once

#include "enums.h"
#include "locationid.h"
#include "pingtime.h"

namespace types {

struct City
{
    LocationID id;
    int idNum;
    QString city;
    QString nick;
    PingTime pingTimeMs;
    bool isPro = false;
    bool isDisabled = false;

    // specific for static IP location
    QString staticIpCountryCode;
    QString staticIpShortName;
    QString staticIpType;
    QString staticIp;

    // specific for custom config location
    CUSTOM_CONFIG_TYPE customConfigType = CUSTOM_CONFIG_OPENVPN;
    bool customConfigIsCorrect = false;
    QString customConfigErrorMessage;

    bool is10Gbps = false;
    int health = 0;     // correct values [0..100]

    bool operator==(const City &other) const;
    bool operator!=(const City &other) const;
};

struct Location
{
    LocationID id;
    int idNum;
    QString name;
    QString countryCode;
    QString shortName;
    bool isPremiumOnly = false;
    bool isNoP2P = false;
    QVector<City> cities;

    bool operator==(const Location &other) const;
    bool operator!=(const Location &other) const;

    // utils functions
    static QVector<Location> loadLocationsFromJson(const QByteArray &arr);
    static Location loadLocationFromJson(const QByteArray &arr);

private:
    static Location locationFromJsonObject(const QJsonObject &obj);

};

} //namespace types
