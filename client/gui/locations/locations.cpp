#include "locations.h"
#include "locationsmodel_roles.h"

namespace gui_location {

Locations::Locations(QObject *parent) : QObject(parent)
{
    connect(&timer_, &QTimer::timeout, this, &Locations::onChangeConnectionSpeedTimer);

    locationsModel_ = new LocationsModel(this);
    sortedLocationsModel_ = new SortedLocationsModel(this);
    sortedLocationsModel_->setSourceModel(locationsModel_);
    sortedLocationsModel_->sort(0);

    citiesModel_ = new CitiesModel(this);
    citiesModel_->setSourceModel(locationsModel_);

    sortedCitiesModel_ = new SortedCitiesModel(this);
    sortedCitiesModel_->setSourceModel(citiesModel_);
    sortedCitiesModel_->sort(0);

    favoriteCitiesModel_ = new FavoriteCitiesModel(this);
    favoriteCitiesModel_->setSourceModel(sortedCitiesModel_);

    staticIpsModel_ = new StaticIpsModel(this);
    staticIpsModel_->setSourceModel(sortedCitiesModel_);

    customConfigsModel_ = new CustomConfgisModel(this);
    customConfigsModel_->setSourceModel(sortedCitiesModel_);
}

void Locations::updateLocations(const LocationID &bestLocation, const QVector<types::LocationItem> &locations)
{
    locationsModel_->updateLocations(bestLocation, locations);
}

void Locations::updateDeviceName(const QString &staticIpDeviceName)
{
    if (staticIpDeviceName_ != staticIpDeviceName)
    {
        staticIpDeviceName_ = staticIpDeviceName;
        emit deviceNameChanged(staticIpDeviceName_);
    }
}

void Locations::updateBestLocation(const LocationID &bestLocation)
{
    locationsModel_->updateBestLocation(bestLocation);
}

void Locations::updateCustomConfigLocation(const types::LocationItem &location)
{
    locationsModel_->updateCustomConfigLocation(location);
}

// since the connection speed change can be called quite often, we limit this processing to once every 0.5 second
void Locations::changeConnectionSpeed(LocationID id, PingTime speed)
{
    connectionSpeeds_[id] = speed;
    if (!timer_.isActive())
    {
        timer_.start(UPDATE_CONNECTION_SPEED_PERIOD);
    }
}

void Locations::setLocationOrder(ORDER_LOCATION_TYPE orderLocationType)
{
    sortedLocationsModel_->setLocationOrder(orderLocationType);
    sortedCitiesModel_->setLocationOrder(orderLocationType);
}

void Locations::setFreeSessionStatus(bool isFreeSessionStatus)
{
    locationsModel_->setFreeSessionStatus(isFreeSessionStatus);
}

QModelIndex Locations::getIndexByLocationId(const LocationID &id) const
{
    return locationsModel_->getIndexByLocationId(id);
}

QModelIndex Locations::getIndexByFilter(const QString &strFilter) const
{
    return locationsModel_->getIndexByFilter(strFilter);
}

LocationID Locations::getBestLocationId() const
{
    QModelIndex mi = locationsModel_->getBestLocationIndex();
    if (mi.isValid())
    {
        return qvariant_cast<LocationID>(mi.data(LOCATION_ID));
    }
    return LocationID();
}

LocationID Locations::getFirstValidCustomConfigLocationId() const
{
    Q_ASSERT(false);
    return LocationID();
}

LocationID Locations::findGenericLocationByTitle(const QString &title) const
{
    Q_ASSERT(false);
    return LocationID();
}

LocationID Locations::findCustomConfigLocationByTitle(const QString &title) const
{
    Q_ASSERT(false);
    return LocationID();
}

int Locations::getNumGenericLocations() const
{
    Q_ASSERT(false);
    return 0;
}

int Locations::getNumFavoriteLocations() const
{
    Q_ASSERT(false);
    return 0;
}

int Locations::getNumStaticIPLocations() const
{
    Q_ASSERT(false);
    return 0;
}

int Locations::getNumCustomConfigLocations() const
{
    Q_ASSERT(false);
    return 0;
}

void Locations::onChangeConnectionSpeedTimer()
{
    for (QHash<LocationID, PingTime>::const_iterator it = connectionSpeeds_.constBegin(); it != connectionSpeeds_.constEnd(); ++it)
    {
        locationsModel_->changeConnectionSpeed(it.key(), it.value());
    }
    connectionSpeeds_.clear();
    timer_.stop();
}



} //namespace gui_location
