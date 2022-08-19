#include "locationsmodel_manager.h"
#include "locationsmodel_roles.h"
#include "model/proxymodels/cities_proxymodel.h"
#include "model/proxymodels/favoritecities_proxymodel.h"
#include "model/proxymodels/staticips_proxymodel.h"
#include "model/proxymodels/customconfigs_proxymodel.h"


namespace gui_locations {

LocationsModelManager::LocationsModelManager(QObject *parent) : QObject(parent)
{
    connect(&timer_, &QTimer::timeout, this, &LocationsModelManager::onChangeConnectionSpeedTimer);

    locationsModel_ = new LocationsModel(this);
    sortedLocationsProxyModel_ = new SortedLocationsProxyModel(this);
    sortedLocationsProxyModel_->setSourceModel(locationsModel_);
    sortedLocationsProxyModel_->sort(0);

    citiesProxyModel_ = new CitiesProxyModel(this);
    citiesProxyModel_->setSourceModel(locationsModel_);

    sortedCitiesProxyModel_ = new SortedCitiesProxyModel(this);
    sortedCitiesProxyModel_->setSourceModel(citiesProxyModel_);
    sortedCitiesProxyModel_->sort(0);

    favoriteCitiesProxyModel_ = new FavoriteCitiesProxyModel(this);
    favoriteCitiesProxyModel_->setSourceModel(sortedCitiesProxyModel_);

    staticIpsProxyModel_ = new StaticIpsProxyModel(this);
    staticIpsProxyModel_->setSourceModel(sortedCitiesProxyModel_);

    customConfigsProxyModel_ = new CustomConfgisProxyModel(this);
    customConfigsProxyModel_->setSourceModel(sortedCitiesProxyModel_);
}

void LocationsModelManager::updateLocations(const LocationID &bestLocation, const QVector<types::Location> &locations)
{
    locationsModel_->updateLocations(bestLocation, locations);
}

void LocationsModelManager::updateDeviceName(const QString &staticIpDeviceName)
{
    if (staticIpDeviceName_ != staticIpDeviceName)
    {
        staticIpDeviceName_ = staticIpDeviceName;
        emit deviceNameChanged(staticIpDeviceName_);
    }
}

void LocationsModelManager::updateBestLocation(const LocationID &bestLocation)
{
    locationsModel_->updateBestLocation(bestLocation);
}

void LocationsModelManager::updateCustomConfigLocation(const types::Location &location)
{
    locationsModel_->updateCustomConfigLocation(location);
}

// since the connection speed change can be called quite often, we limit this processing to once every 0.5 second
void LocationsModelManager::changeConnectionSpeed(LocationID id, PingTime speed)
{
    connectionSpeeds_[id] = speed;
    if (!timer_.isActive())
    {
        timer_.start(UPDATE_CONNECTION_SPEED_PERIOD);
    }
}

void LocationsModelManager::setLocationOrder(ORDER_LOCATION_TYPE orderLocationType)
{
    sortedLocationsProxyModel_->setLocationOrder(orderLocationType);
    sortedCitiesProxyModel_->setLocationOrder(orderLocationType);
}

void LocationsModelManager::setFreeSessionStatus(bool isFreeSessionStatus)
{
    locationsModel_->setFreeSessionStatus(isFreeSessionStatus);
}

QModelIndex LocationsModelManager::getIndexByLocationId(const LocationID &id) const
{
    return locationsModel_->getIndexByLocationId(id);
}

QModelIndex LocationsModelManager::getIndexByFilter(const QString &strFilter) const
{
    return locationsModel_->getIndexByFilter(strFilter);
}

LocationID LocationsModelManager::findLocationByFilter(const QString &strFilter) const
{
    Q_ASSERT(false);
    return LocationID();
}

LocationID LocationsModelManager::getBestLocationId() const
{
    QModelIndex mi = locationsModel_->getBestLocationIndex();
    if (mi.isValid())
    {
        return qvariant_cast<LocationID>(mi.data(LOCATION_ID));
    }
    return LocationID();
}

LocationID LocationsModelManager::getFirstValidCustomConfigLocationId() const
{
    Q_ASSERT(false);
    return LocationID();
}

LocationID LocationsModelManager::findGenericLocationByTitle(const QString &title) const
{
    Q_ASSERT(false);
    return LocationID();
}

LocationID LocationsModelManager::findCustomConfigLocationByTitle(const QString &title) const
{
    Q_ASSERT(false);
    return LocationID();
}

int LocationsModelManager::getNumGenericLocations() const
{
    Q_ASSERT(false);
    return 0;
}

int LocationsModelManager::getNumFavoriteLocations() const
{
    Q_ASSERT(false);
    return 0;
}

int LocationsModelManager::getNumStaticIPLocations() const
{
    Q_ASSERT(false);
    return 0;
}

int LocationsModelManager::getNumCustomConfigLocations() const
{
    Q_ASSERT(false);
    return 0;
}

void LocationsModelManager::onChangeConnectionSpeedTimer()
{
    for (QHash<LocationID, PingTime>::const_iterator it = connectionSpeeds_.constBegin(); it != connectionSpeeds_.constEnd(); ++it)
    {
        locationsModel_->changeConnectionSpeed(it.key(), it.value());
    }
    connectionSpeeds_.clear();
    timer_.stop();
}



} //namespace gui_locations
