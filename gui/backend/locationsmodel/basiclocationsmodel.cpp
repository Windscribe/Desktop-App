#include "basiclocationsmodel.h"
#include "sortlocationsalgorithms.h"

BasicLocationsModel::BasicLocationsModel(QObject *parent) : QObject(parent), orderLocationsType_(ProtoTypes::ORDER_LOCATION_BY_GEOGRAPHY),
    isFreeSessionStatus_(false)
{

}

BasicLocationsModel::~BasicLocationsModel()
{
    clearLocations();
}

void BasicLocationsModel::setOrderLocationsType(ProtoTypes::OrderLocationType orderLocationsType)
{
    orderLocationsType_ = orderLocationsType;
    sort();
    emit itemsUpdated(locations_);
}

void BasicLocationsModel::changeConnectionSpeed(LocationID id, PingTime speed)
{
    const bool location_found = [&]() {
        for (auto *lmi : locations_) {
            for (auto &cmi : lmi->cities) {
                if (cmi.id == id) {
                    cmi.pingTimeMs = speed;
                    return true;
                }
            }
        }
        return false;
    }();

    if (location_found)
        emit connectionSpeedChanged(id, speed);
}

void BasicLocationsModel::setIsFavorite(LocationID id, bool isFavorite)
{
    for (LocationModelItem *lmi : qAsConst(locations_))
    {
        for (int i = 0; i < lmi->cities.count(); ++i)
        {
            if (lmi->cities[i].id == id)
            {
                lmi->cities[i].isFavorite = isFavorite;
                emit isFavoriteChanged(id, isFavorite);
                break;
            }
        }
    }
}

void BasicLocationsModel::setFreeSessionStatus(bool isFreeSessionStatus)
{
    if (isFreeSessionStatus != isFreeSessionStatus_)
    {
        isFreeSessionStatus_ = isFreeSessionStatus;
        emit freeSessionStatusChanged(isFreeSessionStatus_);
    }
}

QVector<LocationModelItem *> BasicLocationsModel::items()
{
    return locations_;
}

void BasicLocationsModel::clearLocations()
{
    for (LocationModelItem *lmi : qAsConst(locations_))
    {
        delete lmi;
    }
    locations_.clear();
}

void BasicLocationsModel::sort()
{
    if (orderLocationsType_ == ProtoTypes::ORDER_LOCATION_BY_GEOGRAPHY)
    {
        std::sort(locations_.begin(), locations_.end(), SortLocationsAlgorithms::lessThanByGeography);
    }
    else if (orderLocationsType_ == ProtoTypes::ORDER_LOCATION_BY_ALPHABETICALLY)
    {
        std::sort(locations_.begin(), locations_.end(), SortLocationsAlgorithms::lessThanByAlphabetically);
    }
    else if (orderLocationsType_ == ProtoTypes::ORDER_LOCATION_BY_LATENCY)
    {
        std::sort(locations_.begin(), locations_.end(), SortLocationsAlgorithms::lessThanByLatency);
    }
    else
    {
        Q_ASSERT(false);
    }
}
