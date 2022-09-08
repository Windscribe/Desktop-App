#include "locationitem.h"

namespace gui_locations {

LocationItem::LocationItem(const types::Location &location) : location_(location), load_(0), averagePing_(0),
    bNeedRecalcInternalValue_(true), is10gbps_(false)
{
}

LocationItem::LocationItem(const LocationID &bestLocation, const types::Location &l, int cityInd)
{
    Q_ASSERT(bestLocation.isBestLocation());
    location_.name = "Best Location";
    location_.id = bestLocation;
    location_.countryCode = l.countryCode;
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
    averagePing_ = city.pingTimeMs.toInt();
    nickname_ = city.nick;
}

int LocationItem::load()
{
    recalcIfNeed();
    return load_;
}

int LocationItem::averagePing()
{
    recalcIfNeed();
    return averagePing_;
}

void LocationItem::insertCityAtInd(int ind, const types::City &city)
{
    location_.cities.insert(ind, city);
    bNeedRecalcInternalValue_ = true;
}

void LocationItem::removeCityAtInd(int ind)
{
    Q_ASSERT(ind >= 0 && ind < location_.cities.size());
    location_.cities.removeAt(ind);
    bNeedRecalcInternalValue_ = true;
}

void LocationItem::updateCityAtInd(int ind, const types::City &city)
{
    Q_ASSERT(ind >= 0 && ind < location_.cities.size());
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
        averagePing_ = time.toInt();
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
           other.averagePing_ == averagePing_ &&
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
        recalcAveragePing();
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

void LocationItem::recalcAveragePing()
{
    // doesn't make calc for CustomConfig and the best location
    if (location_.id.isCustomConfigsLocation() || location_.id.isBestLocation()) {
        return;
    }

    double sumPing = 0;
    int cnt = 0;
    for (int i = 0; i < location_.cities.size(); ++i)
    {
        int cityPing = location_.cities[i].pingTimeMs.toInt();
        if (cityPing == PingTime::NO_PING_INFO)
        {
            sumPing += 200;     // we assume a maximum ping time for three bars
        }
        else if (cityPing == PingTime::PING_FAILED)
        {
            sumPing += 2000;    // 2000 - max ping interval
        }
        else
        {
            sumPing += cityPing;
        }
        cnt++;
    }

    if (cnt > 0)
    {
        averagePing_ = sumPing / (double)cnt;
    }
    else
    {
        averagePing_ = -1;
    }
}

} //namespace gui_locations

