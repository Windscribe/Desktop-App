#include "locationitem.h"

namespace types {

bool LocationItem::operator==(const LocationItem &other) const
{
    return other.id == id &&
           other.name == name &&
           other.countryCode == countryCode &&
           other.isPremiumOnly == isPremiumOnly &&
           other.isP2P == isP2P &&
           other.cities == cities;
}

bool LocationItem::operator!=(const LocationItem &other) const
{
    return !(*this == other);
}

bool CityItem::operator==(const CityItem &other) const
{
    return other.id == id &&
           other.city == city &&
           other.nick == nick &&
           other.pingTimeMs == pingTimeMs &&
           other.isPro == isPro &&
           other.isDisabled == isDisabled &&
           other.staticIpCountryCode == staticIpCountryCode &&
           other.staticIpType == staticIpType &&
           other.staticIp == staticIp &&
           other.customConfigType == customConfigType &&
           other.customConfigIsCorrect == customConfigIsCorrect &&
           other.customConfigErrorMessage == customConfigErrorMessage &&
           other.linkSpeed == linkSpeed &&
           other.health == health;
}

bool CityItem::operator!=(const CityItem &other) const
{
    return !(*this == other);
}



} //namespace types

