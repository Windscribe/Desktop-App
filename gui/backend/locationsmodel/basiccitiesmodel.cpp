#include "basiccitiesmodel.h"

BasicCitiesModel::BasicCitiesModel(QObject *parent) : QObject(parent), isFreeSessionStatus_(false)
{

}

BasicCitiesModel::~BasicCitiesModel()
{
    clearCities();
}

void BasicCitiesModel::setOrderLocationsType(ProtoTypes::OrderLocationType orderLocationsType)
{
    orderLocationsType_ = orderLocationsType;
    sort(cities_);
    emit itemsUpdated(cities_);
}

void BasicCitiesModel::changeConnectionSpeed(const LocationID &id, const PingTime &speed)
{
    for (CityModelItem *cmi : qAsConst(cities_))
    {
        if (cmi->id == id)
        {
            cmi->pingTimeMs = speed;
            emit connectionSpeedChanged(id, speed);
            break;
        }
    }
}

void BasicCitiesModel::setIsFavorite(const LocationID &id, bool isFavorite)
{
    for (CityModelItem *cmi : qAsConst(cities_))
    {
        if (cmi->id == id)
        {
            cmi->isFavorite = isFavorite;
            emit isFavoriteChanged(id, isFavorite);
            break;
        }
    }
}

void BasicCitiesModel::setFreeSessionStatus(bool isFreeSessionStatus)
{
    if (isFreeSessionStatus != isFreeSessionStatus_)
    {
        isFreeSessionStatus_ = isFreeSessionStatus;
        emit freeSessionStatusChanged(isFreeSessionStatus_);
    }
}

QVector<CityModelItem *> BasicCitiesModel::getCities()
{
    return cities_;
}

void BasicCitiesModel::clearCities()
{
    for (CityModelItem *cmi : qAsConst(cities_))
    {
        delete cmi;
    }
    cities_.clear();
}

void BasicCitiesModel::sort(QVector<CityModelItem *> &cities)
{
    Q_UNUSED(cities);
}
