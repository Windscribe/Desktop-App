#include "locationitem_wrapper.h"

namespace gui_location {

LocationItemWrapper::LocationItemWrapper(const types::LocationItem &location) : location_(location), load_(0), averagePing_(0), is10gbps_(false)
{
    recalcLoad();
    recalcAveragePing();
}

LocationItemWrapper::LocationItemWrapper(const LocationID &bestLocation, const types::LocationItem &l, int cityInd)
{
    const types::CityItem &city = l.cities[cityInd];

    location_.name = "Best Location";
    location_.id = bestLocation;
    location_.countryCode = l.countryCode;
    location_.isNoP2P = l.isNoP2P;
    location_.isPremiumOnly = l.isPremiumOnly;
    is10gbps_ = (city.linkSpeed == 10000);

    if (city.health >= 0 && city.health <= 100)
    {
        load_ = city.health;
    }
    else
    {
        load_ = 0;
    }
    averagePing_ = city.pingTimeMs.toInt();
}

QString LocationItemWrapper::nickname() const
{
    // makes sense only for the best location
    Q_ASSERT(location_.id.isBestLocation());
    if (location_.id.isBestLocation())
    {
        Q_ASSERT(location_.cities.size() == 1);
        if (location_.cities.size() == 1)
        {
            return location_.cities[0].nick;
        }
    }
    return QString();
}

bool LocationItemWrapper::operator==(const LocationItemWrapper &other) const
{
    return other.location_ == location_ &&
           other.load_ == load_ &&
           other.averagePing_ == averagePing_ &&
           other.is10gbps_ == is10gbps_;
}

bool LocationItemWrapper::operator!=(const LocationItemWrapper &other) const
{
    return !(*this == other);
}

QVector<int> LocationItemWrapper::findRemovedLocations(const QVector<LocationItemWrapper> &original, const QVector<LocationItemWrapper> &changed)
{
    QVector<int> v;
    for (int ind = 0; ind < original.size(); ++ind)
    {
        LocationID lid = original[ind].location_.id;
        if (std::find_if(changed.begin(), changed.end(),
                     [&](const LocationItemWrapper &l) {
                        return lid == l.location_.id;
                    }) == std::end(changed))
        {
            v << ind;
        }
    }
    return v;
}

QVector<int> LocationItemWrapper::findNewLocations(const QVector<LocationItemWrapper> &original, const QVector<LocationItemWrapper> &changed)
{
    return findRemovedLocations(changed, original);
}

void LocationItemWrapper::recalcLoad()
{
    // doesn't make calc for CustomConfig location
    if (location_.id.isCustomConfigsLocation()) {
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

void LocationItemWrapper::recalcAveragePing()
{
    // doesn't make calc for CustomConfig location
    if (location_.id.isCustomConfigsLocation())
    {
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



} //namespace gui_location

