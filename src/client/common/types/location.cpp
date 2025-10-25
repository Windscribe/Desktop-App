#include "location.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "utils/ws_assert.h"

namespace types {

bool City::operator==(const City &other) const
{
    return other.id == id &&
           other.city == city &&
           other.nick == nick &&
           other.pingTimeMs == pingTimeMs &&
           other.isPro == isPro &&
           other.isDisabled == isDisabled &&
           other.staticIpCountryCode == staticIpCountryCode &&
           other.staticIpShortName == staticIpShortName &&
           other.staticIpType == staticIpType &&
           other.staticIp == staticIp &&
           other.customConfigType == customConfigType &&
           other.customConfigIsCorrect == customConfigIsCorrect &&
           other.customConfigErrorMessage == customConfigErrorMessage &&
           other.is10Gbps == is10Gbps &&
           other.health == health;
}

bool City::operator!=(const City &other) const
{
    return !(*this == other);
}

bool Location::operator==(const Location &other) const
{
    return other.id == id &&
           other.name == name &&
           other.countryCode == countryCode &&
           other.shortName == shortName &&
           other.isPremiumOnly == isPremiumOnly &&
           other.isNoP2P == isNoP2P &&
           other.cities == cities;
}

bool Location::operator!=(const Location &other) const
{
    return !(*this == other);
}

QVector<Location> Location::loadLocationsFromJson(const QByteArray &arr)
{
    QVector<Location> locations;
    QJsonDocument doc = QJsonDocument::fromJson(arr);
    WS_ASSERT(doc.isObject());
    QJsonArray jsonArr = doc.object()["locations"].toArray();
    WS_ASSERT(!jsonArr.isEmpty());
    for (const auto &item : jsonArr)
    {
        locations << locationFromJsonObject(item.toObject());
    }

    return locations;
}

Location Location::loadLocationFromJson(const QByteArray &arr)
{
    QJsonDocument doc = QJsonDocument::fromJson(arr);
    WS_ASSERT(doc.isObject());
    return locationFromJsonObject(doc.object());
}

Location Location::locationFromJsonObject(const QJsonObject &obj)
{
    Location location;
    location.id = LocationID(obj["id"].toObject()["type"].toInt(),
                             obj["id"].toObject()["id"].toInt(),
                             obj["id"].toObject()["city"].toString());
    location.name = obj["name"].toString();
    location.countryCode = obj["country_code"].toString();
    location.shortName = obj["short_name"].toString();
    location.isPremiumOnly = obj["is_premium_only"].toBool();
    location.isNoP2P = !obj["is_p2p_supported"].toBool();

    for (const auto &c : obj["cities"].toArray())
    {
        City city;
        QJsonObject objCity = c.toObject();
        city.id = LocationID(objCity["id"].toObject()["type"].toInt(),
                objCity["id"].toObject()["id"].toInt(),
                objCity["id"].toObject()["city"].toString());
        city.city = objCity["name"].toString();
        city.nick = objCity["nick"].toString();
        city.pingTimeMs = objCity["ping_time"].toInt();
        city.isPro = objCity["is_premium_only"].toBool();
        city.isDisabled = objCity["is_disabled"].toBool();
        city.staticIpCountryCode = objCity["static_ip_country_code"].toString();
        city.staticIpShortName = objCity["static_ip_short_name"].toString();
        city.staticIpType = objCity["static_ip_type"].toString();
        city.staticIp = objCity["static_ip"].toString();
        city.customConfigType = (objCity["custom_config_type"].toString() == "CUSTOM_CONFIG_OPENVPN" ? CUSTOM_CONFIG_OPENVPN : CUSTOM_CONFIG_WIREGUARD);
        city.customConfigIsCorrect = objCity["custom_config_is_correct"].toBool();
        city.customConfigErrorMessage = objCity["custom_config_error_message"].toString();
        city.is10Gbps = (objCity["link_speed"].toInt() == 10000);
        city.health = objCity["health"].toInt();
        location.cities << city;
    }
    return location;
}

} // namespace types
