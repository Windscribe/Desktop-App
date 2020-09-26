#include "locationsmodel.h"

#include "alllocationsmodel.h"
#include "configuredcitiesmodel.h"
#include "staticipscitiesmodel.h"
#include "favoritecitiesmodel.h"
#include "sortlocationsalgorithms.h"

#include <QFile>


LocationsModel::LocationsModel(QObject *parent) : QObject(parent)
  , deviceName_("")
{
    favoriteLocationsStorage_.readFromSettings();
    allLocations_ = new AllLocationsModel(this);
    configuredLocations_ = new ConfiguredCitiesModel(this);
    staticIpsLocations_ = new StaticIpsCitiesModel(this);
    favoriteLocations_ = new FavoriteCitiesModel(this);
}

LocationsModel::~LocationsModel()
{
    for (const LocationModelItem *lmi : locations_)
    {
        delete lmi;
    }
    favoriteLocationsStorage_.writeToSettings();
}

void LocationsModel::update(const ProtoTypes::LocationId &bestLocation, const ProtoTypes::ArrayLocations &locations)
{
    for (const LocationModelItem *lmi : locations_)
    {
        delete lmi;
    }
    locations_.clear();

    bestLocationId_ = LocationID::createFromProtoBuf(bestLocation);

    int cnt = locations.locations_size();
    for (int i = 0; i < cnt; ++i)
    {
        const ProtoTypes::Location &location = locations.locations(i);

        LocationModelItem *lmi = new LocationModelItem();
        lmi->initialInd_ = i;
        lmi->id = LocationID::createFromProtoBuf(location.id());
        lmi->title = QString::fromStdString(location.name());
        lmi->isShowP2P = location.is_p2p_supported();
        lmi->countryCode = QString::fromStdString(location.country_code()).toLower();
        lmi->isPremiumOnly = location.is_premium_only();

        int cities_cnt = location.cities_size();
        for (int c = 0; c < cities_cnt; ++c)
        {
            const ProtoTypes::City &city = location.cities(c);
            CityModelItem cmi;
            cmi.id = LocationID::createFromProtoBuf(city.id());
            cmi.city = QString::fromStdString(city.name());
            cmi.nick = QString::fromStdString(city.nick());
            cmi.countryCode = lmi->id.isStaticIpsLocation() ? QString::fromStdString(city.static_ip_country_code()) : lmi->countryCode;
            cmi.pingTimeMs = city.ping_time();
            cmi.bShowPremiumStarOnly = city.is_premium_only();
            cmi.isFavorite = favoriteLocationsStorage_.isFavorite(cmi.id);
            cmi.isDisabled = city.is_disabled();
            cmi.staticIpCountryCode = QString::fromStdString(city.static_ip_country_code());
            cmi.staticIpType = QString::fromStdString(city.static_ip_type());
            cmi.staticIp = QString::fromStdString(city.static_ip());
            lmi->cities << cmi;
        }

        // sort cities alphabetically
        std::sort(lmi->cities.begin(), lmi->cities.end(), SortLocationsAlgorithms::lessThanByAlphabeticallyCityItem);

        locations_ << lmi;

        // if this is the best location then insert copy to top list
        if (!lmi->id.isStaticIpsLocation() && !lmi->id.isCustomConfigsLocation() && lmi->id.apiLocationToBestLocation() == bestLocationId_)
        {
            LocationModelItem *lmiBestLocation = new LocationModelItem();
            *lmiBestLocation = *lmi;
            lmiBestLocation->id = bestLocationId_;
            lmiBestLocation->title = tr("Best Location");
            for (int c = 0; c < lmiBestLocation->cities.count(); ++c)
            {
                lmiBestLocation->cities[c].id = lmiBestLocation->cities[c].id.apiLocationToBestLocation();
            }
            locations_.insert(0, lmiBestLocation);
        }

        if (lmi->id.isStaticIpsLocation())
        {
            deviceName_ = QString::fromStdString(location.static_ip_device_name());
            if (!deviceName_.isEmpty())
            {
                emit deviceNameChanged(deviceName_);
            }
        }
    }

    allLocations_->update(locations_);
    //configuredLocations_->update(locations_);
    staticIpsLocations_->update(locations_);
    favoriteLocations_->update(locations_);
}

BasicLocationsModel *LocationsModel::getAllLocationsModel()
{
    return allLocations_;
}

BasicCitiesModel *LocationsModel::getConfiguredLocationsModel()
{
    return configuredLocations_;
}

BasicCitiesModel *LocationsModel::getStaticIpsLocationsModel()
{
    return staticIpsLocations_;
}

BasicCitiesModel *LocationsModel::getFavoriteLocationsModel()
{
    return favoriteLocations_;
}

void LocationsModel::setOrderLocationsType(ProtoTypes::OrderLocationType orderLocationsType)
{
    if (orderLocationsType != orderLocationsType_)
    {
        orderLocationsType_ = orderLocationsType;
        allLocations_->setOrderLocationsType(orderLocationsType_);
        configuredLocations_->setOrderLocationsType(orderLocationsType_);
        staticIpsLocations_->setOrderLocationsType(orderLocationsType_);
        favoriteLocations_->setOrderLocationsType(orderLocationsType_);
    }
}

void LocationsModel::switchFavorite(const LocationID &id, bool isFavorite)
{
    if (isFavorite)
    {
        favoriteLocationsStorage_.addToFavorites(id);
    }
    else
    {
        favoriteLocationsStorage_.removeFromFavorites(id);
    }
    allLocations_->setIsFavorite(id, isFavorite);
    configuredLocations_->setIsFavorite(id, isFavorite);
    staticIpsLocations_->setIsFavorite(id, isFavorite);
    favoriteLocations_->setIsFavorite(id, isFavorite);
}

bool LocationsModel::getLocationInfo(const LocationID &id, LocationsModel::LocationInfo &li)
{
    for (int i = 0; i < locations_.count(); ++i)
    {
        if (locations_[i]->id == id)
        {
            if (locations_[i]->cities.count() > 0)
            {
                li.id = id;
                li.firstName = locations_[i]->cities[0].city;
                li.secondName = locations_[i]->cities[0].nick;
                li.countryCode = locations_[i]->countryCode;
                li.pingTime = locations_[i]->cities[0].pingTimeMs;
                return true;
            }
        }
        for (int k = 0; k < locations_[i]->cities.count(); ++k)
        {
            if (locations_[i]->cities[k].id == id)
            {
                li.id = id;
                li.firstName = locations_[i]->cities[k].city;
                li.secondName = locations_[i]->cities[k].nick;
                li.countryCode = locations_[i]->cities[k].countryCode;
                li.pingTime = locations_[i]->cities[k].pingTimeMs;
                return true;
            }
        }
    }
    return false;
}

QList<CityModelItem> LocationsModel::activeCityModelItems()
{
    QList<CityModelItem> cities;

    for (int  i = 0; i < locations_.count(); ++i)
    {
        LocationModelItem *lmi = locations_[i];
        LocationID id = lmi->id;

        for (int k = 0; k < locations_[i]->cities.count(); ++k)
        {
            // if (!lmi->cities[k].isDisabled)
            {
                cities.append(lmi->cities[k]);
            }
        }
    }
    return cities;
}

QVector<LocationModelItem *> LocationsModel::locationModelItems()
{
    return locations_;
}

void LocationsModel::setFreeSessionStatus(bool isFreeSessionStatus)
{
    allLocations_->setFreeSessionStatus(isFreeSessionStatus);
    configuredLocations_->setFreeSessionStatus(isFreeSessionStatus);
    staticIpsLocations_->setFreeSessionStatus(isFreeSessionStatus);
    favoriteLocations_->setFreeSessionStatus(isFreeSessionStatus);
}

void LocationsModel::changeConnectionSpeed(LocationID id, PingTime speed)
{
    [&]() {
        for (auto *lmi : locations_) {
            for (auto &cmi : lmi->cities) {
                if (cmi.id == id) {
                    cmi.pingTimeMs = speed;
                    return;
                }
            }
        }
    }();

    allLocations_->changeConnectionSpeed(id, speed);
    configuredLocations_->changeConnectionSpeed(id, speed);
    staticIpsLocations_->changeConnectionSpeed(id, speed);
    favoriteLocations_->changeConnectionSpeed(id, speed);
    emit locationSpeedChanged(id, speed);
}

// example of location string: NL, Toronto #1, etc
LocationID LocationsModel::getLocationIdByName(const QString &location) const
{
    for (const LocationModelItem * lmi: locations_)
    {
        if (lmi->countryCode.compare(location, Qt::CaseInsensitive) == 0)
        {
            return LocationID(lmi->id);
        }
        for (const CityModelItem &city: lmi->cities)
        {
            if (city.city.compare(location, Qt::CaseInsensitive) == 0)
            {
                return city.id;
            }
        }
    }

    return LocationID();
}

LocationID LocationsModel::getBestLocationId() const
{
    Q_ASSERT(bestLocationId_.isValid());
    return bestLocationId_;
}
