#include "locationsmodel_manager.h"
#include "locationsmodel_roles.h"
#include "model/proxymodels/cities_proxymodel.h"
#include "model/proxymodels/favoritecities_proxymodel.h"
#include "model/proxymodels/staticips_proxymodel.h"
#include "model/proxymodels/customconfigs_proxymodel.h"
#include "utils/utils.h"

namespace gui_locations {

LocationsModelManager::LocationsModelManager(QObject *parent) : QObject(parent)
{
    connect(&timer_, &QTimer::timeout, this, &LocationsModelManager::onChangeConnectionSpeedTimer);

    locationsModel_ = new LocationsModel(this);
    sortedLocationsProxyModel_ = new SortedLocationsProxyModel(this);
    sortedLocationsProxyModel_->setSourceModel(locationsModel_);
    sortedLocationsProxyModel_->sort(0);

    filterLocationsProxyModel_ = new SortedLocationsProxyModel(this);
    filterLocationsProxyModel_->setSourceModel(locationsModel_);
    filterLocationsProxyModel_->sort(0);

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
    filterLocationsProxyModel_->setLocationOrder(orderLocationType);
}

void LocationsModelManager::setFreeSessionStatus(bool isFreeSessionStatus)
{
    locationsModel_->setFreeSessionStatus(isFreeSessionStatus);
}

QModelIndex LocationsModelManager::getIndexByLocationId(const LocationID &id) const
{
    return locationsModel_->getIndexByLocationId(id);
}

LocationID LocationsModelManager::findLocationByFilter(const QString &strFilter) const
{
    QString strFilterLower = strFilter.toLower();

    auto getEnabledCities = [this](const QModelIndex &mi) {
        QList<LocationID> citiesIds;
        for (int i = 0, rowCount = locationsModel_->rowCount(mi); i < rowCount; ++i) {
            QModelIndex cityInd = locationsModel_->index(i, 0, mi);
            if (!cityInd.data(kIsDisabled).toBool() && !cityInd.data(kIsShowAsPremium).toBool()) {
                    citiesIds << qvariant_cast<LocationID>(cityInd.data(kLocationId));
            }
        }
        return citiesIds;
    };

    // try find by region(location title) first
    for (int i = 0, rowCount = locationsModel_->rowCount(); i < rowCount; ++i) {
        QModelIndex mi = locationsModel_->index(i, 0);
        LocationID lid = qvariant_cast<LocationID>(mi.data(kLocationId));
        if (!lid.isBestLocation() && !lid.isStaticIpsLocation() && !lid.isCustomConfigsLocation() && mi.data().toString().toLower() == strFilterLower) {
            QList<LocationID> citiesIds = getEnabledCities(mi);
            if (citiesIds.length() > 0) {
                int ind = Utils::generateIntegerRandom(0, citiesIds.length()-1);
                return citiesIds[ind];
            } else {
                return LocationID();
            }
        }
    }

    // try find by country code
    QList<LocationID> citiesWithCountryCode;
    for (int i = 0, rowCount = locationsModel_->rowCount(); i < rowCount; ++i) {
        QModelIndex mi = locationsModel_->index(i, 0);
        LocationID lid = qvariant_cast<LocationID>(mi.data(kLocationId));
        if (!lid.isBestLocation() && !lid.isStaticIpsLocation() && !lid.isCustomConfigsLocation() && mi.data(kCountryCode).toString().toLower() == strFilterLower) {
            citiesWithCountryCode << getEnabledCities(mi);
        }
    }
    if (citiesWithCountryCode.length() > 0) {
        int ind = Utils::generateIntegerRandom(0, citiesWithCountryCode.length()-1);
        return citiesWithCountryCode[ind];
    }

    // try find by city name or nickname
    QVector<LocationID> cmiFoundCity;
    QVector<LocationID> cmiFoundNickname;

    for (int i = 0, rowCount = locationsModel_->rowCount(); i < rowCount; ++i) {
        QModelIndex mi = locationsModel_->index(i, 0);
        LocationID lid = qvariant_cast<LocationID>(mi.data(kLocationId));
        if (!lid.isBestLocation() && !lid.isStaticIpsLocation() && !lid.isCustomConfigsLocation()) {

            for (int c = 0, citiesCount = locationsModel_->rowCount(mi); c < citiesCount; ++c) {
                QModelIndex cityMi = locationsModel_->index(c, 0, mi);

                if (!cityMi.data(kIsDisabled).toBool() && !cityMi.data(kIsShowAsPremium).toBool()) {
                    if (cityMi.data(kName).toString().toLower() == strFilterLower)
                        cmiFoundCity << qvariant_cast<LocationID>(cityMi.data(kLocationId));
                    else if (cityMi.data(kNick).toString().toLower() == strFilterLower)
                        cmiFoundNickname << qvariant_cast<LocationID>(cityMi.data(kLocationId));
                }
            }
        }
    }

    if (!cmiFoundCity.isEmpty()) {
        int ind = Utils::generateIntegerRandom(0, cmiFoundCity.length()-1);
        return cmiFoundCity[ind];
    }

    if (!cmiFoundNickname.isEmpty()) {
        int ind = Utils::generateIntegerRandom(0, cmiFoundNickname.length()-1);
        return cmiFoundNickname[ind];
    }
    return LocationID();
}

LocationID LocationsModelManager::getBestLocationId() const
{
    QModelIndex mi = locationsModel_->getBestLocationIndex();
    if (mi.isValid())
        return qvariant_cast<LocationID>(mi.data(Roles::kLocationId));

    return LocationID();
}

LocationID LocationsModelManager::getFirstValidCustomConfigLocationId() const
{
    QModelIndex mi = locationsModel_->getCustomConfigLocationIndex();
    if (mi.isValid()) {
        int cnt = locationsModel_->rowCount(mi);
        for (int i = 0; i < cnt; ++i) {
            QModelIndex childMi = locationsModel_->index(i, 0, mi);
            if (childMi.data(Roles::kIsCustomConfigCorrect).toBool() && !childMi.data(Roles::kIsDisabled).toBool()) {
                return qvariant_cast<LocationID>(childMi.data(Roles::kLocationId));
            }
        }
    }

    return LocationID();
}

void LocationsModelManager::setFilterString(const QString &filterString)
{
    filterLocationsProxyModel_->setFilter(filterString);
}

void LocationsModelManager::saveFavoriteLocations()
{
    locationsModel_->saveFavoriteLocations();
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

QJsonObject LocationsModelManager::renamedLocations() const
{
    return locationsModel_->renamedLocations();
}

void LocationsModelManager::setRenamedLocations(const QJsonObject &obj)
{
    locationsModel_->setRenamedLocations(obj);
}

void LocationsModelManager::resetRenamedLocations()
{
    locationsModel_->resetRenamedLocations();
}

} //namespace gui_locations
