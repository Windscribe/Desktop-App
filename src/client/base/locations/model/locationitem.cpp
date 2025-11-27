#include "locationitem.h"

#include "languagecontroller.h"

namespace gui_locations {

LocationItem::LocationItem(const types::Location &location) : location_(location), load_(0), lowestPing_(0),
    bNeedRecalcInternalValue_(true), is10gbps_(false)
{
}

LocationItem::LocationItem(const LocationID &bestLocation, const types::Location &l, int cityInd)
{
    WS_ASSERT(bestLocation.isBestLocation());
    location_.id = bestLocation;
    location_.countryCode = l.countryCode;
    location_.shortName = l.shortName;
    location_.isNoP2P = l.isNoP2P;
    location_.isPremiumOnly = l.isPremiumOnly;

    types::City city = l.cities[cityInd];
    is10gbps_ = city.is10Gbps;
    if (city.health >= 0 && city.health <= 100)
    {
        load_ = city.health;
    }
    else
    {
        load_ = 0;
    }
    lowestPing_ = city.pingTimeMs.toInt();
    nickname_ = city.nick;
}

void LocationItem::setName(const QString &name)
{
    WS_ASSERT(location_.id.isBestLocation());
    location_.name = name;
}

int LocationItem::load()
{
    recalcIfNeed();
    return load_;
}

int LocationItem::lowestPing()
{
    recalcIfNeed();
    return lowestPing_;
}

void LocationItem::insertCityAtInd(int ind, const types::City &city)
{
    location_.cities.insert(ind, city);
    bNeedRecalcInternalValue_ = true;
}

void LocationItem::removeCityAtInd(int ind)
{
    WS_ASSERT(ind >= 0 && ind < location_.cities.size());
    location_.cities.removeAt(ind);
    bNeedRecalcInternalValue_ = true;
}

void LocationItem::updateCityAtInd(int ind, const types::City &city)
{
    WS_ASSERT(ind >= 0 && ind < location_.cities.size());
    location_.cities[ind] = city;
    bNeedRecalcInternalValue_ = true;
}

void LocationItem::updateLocation(const types::Location &location)
{
    location_ = location;
    bNeedRecalcInternalValue_ = true;
}

void LocationItem::moveCity(int from, int to)
{
    location_.cities.move(from, to);
}

void LocationItem::setPingTimeForCity(int cityInd, PingTime time)
{
    if (location_.id.isBestLocation())
    {
        lowestPing_ = time.toInt();
    }
    else
    {
        location_.cities[cityInd].pingTimeMs = time;
        bNeedRecalcInternalValue_ = true;
    }
}

bool LocationItem::operator==(const LocationItem &other) const
{
    return other.load_ == load_ &&
           other.lowestPing_ == lowestPing_ &&
           other.location_ == location_ &&
           other.is10gbps_ == is10gbps_ &&
           other.nickname_ == nickname_;
}

bool LocationItem::operator!=(const LocationItem &other) const
{
    return !(*this == other);
}

void LocationItem::recalcIfNeed()
{
    if (bNeedRecalcInternalValue_)
    {
        recalcLoad();
        recalcLowestPing();
        bNeedRecalcInternalValue_ = false;
    }
}

void LocationItem::recalcLoad()
{
    // doesn't make calc for CustomConfig and the best location
    if (location_.id.isCustomConfigsLocation() || location_.id.isBestLocation()) {
        return;
    }

    qreal locationLoadSum = 0.0;
    int locationLoadCount = 0;
    for (int i = 0; i < location_.cities.size(); ++i)
    {
        int cityLoad = location_.cities[i].health;
        if (cityLoad >= 0 && cityLoad <= 100)
        {
            locationLoadSum += cityLoad;
            locationLoadCount += 1;
        }
    }

    if (locationLoadCount > 0)
    {
        load_ = qRound(locationLoadSum / locationLoadCount);
    }
    else
    {
        load_ = 0;
    }
}

void LocationItem::recalcLowestPing()
{
    if (location_.id.isCustomConfigsLocation() || location_.id.isBestLocation()) {
        return;
    }

    int minPing = -1;
    for (int i = 0; i < location_.cities.size(); ++i) {
        int cityPing = location_.cities[i].pingTimeMs.toInt();
        if (cityPing == PingTime::NO_PING_INFO || cityPing == PingTime::PING_FAILED) {
            continue;
        }
        if (minPing == -1 || cityPing < minPing) {
            minPing = cityPing;
        }
    }

    lowestPing_ = minPing;
}

} //namespace gui_locations

