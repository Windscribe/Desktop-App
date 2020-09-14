#include "locationsmodel.h"

#include "alllocationsmodel.h"
#include "configuredcitiesmodel.h"
#include "staticipscitiesmodel.h"
#include "favoritecitiesmodel.h"

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
    Q_FOREACH(LocationModelItem *lmi, locations_)
    {
        delete lmi;
    }
    favoriteLocationsStorage_.writeToSettings();
}

void LocationsModel::update(const ProtoTypes::ArrayLocations &arr)
{
    for (const LocationModelItem *lmi : locations_)
    {
        delete lmi;
    }
    locations_.clear();
    ids.clear();

    int cnt = arr.locations_size();
    for (int i = 0; i < cnt; ++i)
    {
        const ProtoTypes::Location &location = arr.locations(i);

        LocationModelItem *lmi = new LocationModelItem();

        lmi->initialInd_ = i;
        lmi->id = LocationID(location.id());
        lmi->title = QString::fromStdString(location.name());
        lmi->countryCode = QString::fromStdString(location.country_code()).toLower();
        lmi->isShowP2P = location.is_p2p_supported();
        lmi->isPremiumOnly = location.is_premium_only();
    }

    /*QHash<int, int> hash;

    Q_FOREACH(LocationModelItem *lmi, locations_)
    {
        delete lmi;
    }
    locations_.clear();
    ids.clear();

    int cnt = arr.locations_size();
    for (int i = 0; i < cnt; ++i)
    {
        ProtoTypes::Location pl = arr.locations(i);

        if (pl.city_name().empty())
        {
            LocationModelItem *lmi = new LocationModelItem();
            lmi->id = LocationID(pl.id());
            lmi->initialInd_ = i;
            lmi->title = QString::fromStdString(pl.name());
            lmi->countryCode = QString::fromStdString(pl.country_code()).toLower();
            lmi->isShowP2P = pl.is_p2p_supported();
            lmi->isPremiumOnly = pl.is_premium_only();
            lmi->isForceExpand = true;
            lmi->pingTimeMs = pl.pingtime();
            locations_ << lmi;
            hash[pl.id()] = locations_.size() - 1;
        }
        else
        {
            auto it = hash.find(pl.id());

            if (it == hash.end())
            {
                //Q_ASSERT(false);
                continue;
            }
            else
            {
                CityModelItem cmi;

                QString n1, n2;
                splitCityName(QString::fromStdString(pl.city_name()), n1, n2);

                cmi.id = LocationID(pl.id(), QString::fromStdString(pl.city_id()));
                cmi.isFavorite = favoriteLocationsStorage_.isFavorite(cmi.id);
                cmi.isDisabled = pl.is_disabled();
                cmi.title1 = n1;
                cmi.title2 = n2;
                cmi.countryCode = QString::fromStdString(pl.country_code()).toLower();
                cmi.bShowPremiumStarOnly = pl.is_show_premium_star_only();
                cmi.pingTimeMs = pl.pingtime();
                cmi.staticIpType = QString::fromStdString(pl.static_ip_type());
                cmi.staticIp = QString::fromStdString(pl.static_ip());

                locations_[it.value()]->cities << cmi;
                ids << cmi.id;
            }
        }

        // get device name
        // if (deviceName_ == "")
        {
            QString thisLocationsDeviceName = QString::fromStdString(pl.static_ip_device_name());
            if (thisLocationsDeviceName != "")
            {
                deviceName_ = thisLocationsDeviceName;
                emit deviceNameChanged(deviceName_);
            }
        }
    }

    allLocations_->update(locations_);
    configuredLocations_->update(locations_);
    staticIpsLocations_->update(locations_);
    favoriteLocations_->update(locations_);*/
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

void LocationsModel::switchFavorite(LocationID id, bool isFavorite)
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

bool LocationsModel::getLocationInfo(LocationID id, LocationsModel::LocationInfo &li)
{
    for (int  i = 0; i < locations_.count(); ++i)
    {
        if (locations_[i]->id.getId() == id.getId())
        {
            if (id.getCity().isEmpty())
            {
                if (locations_[i]->cities.count() > 0)
                {
                    li.id = id;
                    li.firstName = locations_[i]->cities[0].title1;
                    li.secondName = locations_[i]->cities[0].title2;
                    li.countryCode = locations_[i]->countryCode;
                    li.pingTime = locations_[i]->cities[0].pingTimeMs;
                    li.isFavorite = false;
                    return true;
                }
                else
                {
                    Q_ASSERT(false);
                }
            }
            else
            {
                for (int k = 0; k < locations_[i]->cities.count(); ++k)
                {
                    if (locations_[i]->cities[k].id == id)
                    {
                        if (locations_[i]->id.getId() == LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID)
                        {
                            li.id = id;
                            li.firstName = "Custom Config";
                            li.secondName = locations_[i]->cities[k].title1;
                            li.countryCode = "";
                            li.pingTime = locations_[i]->cities[k].pingTimeMs;
                            li.isFavorite = false;
                        }
                        else
                        {
                            li.id = id;
                            li.firstName = locations_[i]->cities[k].title1;
                            li.secondName = locations_[i]->cities[k].title2;
                            li.countryCode = locations_[i]->cities[k].countryCode;
                            li.pingTime = locations_[i]->cities[k].pingTimeMs;
                            li.isFavorite = false;
                        }
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

QString LocationsModel::countryCodeOfStaticCity(const QString &cityName)
{
    QString countryCode = "";

    QList<CityModelItem> cities;

    for (int  i = 0; i < locations_.count(); ++i)
    {
        LocationModelItem *lmi = locations_[i];

        if (lmi->id.getId() >= 0) //  only look for real locations
        {
            LocationID id = lmi->id;

            for (int k = 0; k < locations_[i]->cities.count(); ++k)
            {
                if (lmi->cities[k].title1 == cityName)
                {
                    countryCode = lmi->cities[k].countryCode;
                    break;
                }
            }
        }
    }

    return countryCode;
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
            if (lmi->id == id) {
                lmi->pingTimeMs = speed;
                return;
            }
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
            if (city.title1.compare(location, Qt::CaseInsensitive) == 0)
            {
                return city.id;
            }
        }
    }

    return LocationID();
}

LocationID LocationsModel::getLocationIdByCity(
    const QString &cityname, bool get_best_location) const
{
    QString title1, title2;
    splitCityName(cityname, title1, title2);

    if (!title1.isEmpty() && !title2.isEmpty()) {
        for (const LocationModelItem * lmi : locations_)
        {
            if (lmi->id.getId() < LocationID::BEST_LOCATION_ID)
                continue;
            const bool is_best_location = lmi->id.getId() == LocationID::BEST_LOCATION_ID;
            if (is_best_location != get_best_location)
                continue;
            for (const CityModelItem &city : lmi->cities)
            {
                if (city.title1.compare(title1, Qt::CaseInsensitive) == 0 &&
                    city.title2.compare(title2, Qt::CaseInsensitive) == 0)
                {
                    return city.id;
                }
            }
        }
    }

    return LocationID();
}

void LocationsModel::splitCityName(const QString &src, QString &outName1, QString &outName2) const
{
    QStringList strs = src.split(" - ");
    if (strs.count() > 0)
    {
        outName1 = strs[0].trimmed();
    }
    if (strs.count() > 1)
    {
        outName2 = strs[1].trimmed();
    }
}
