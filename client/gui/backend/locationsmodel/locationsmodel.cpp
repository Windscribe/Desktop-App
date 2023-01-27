#include "locationsmodel.h"

#include "alllocationsmodel.h"
#include "configuredcitiesmodel.h"
#include "staticipscitiesmodel.h"
#include "favoritecitiesmodel.h"
#include "sortlocationsalgorithms.h"
#include "utils/utils.h"

#include <QFile>
#include <QSharedPointer>


LocationsModel::LocationsModel(QObject *parent) : QObject(parent),
    numStaticIPLocations_(0), numStaticIPLocationCities_(0)
{
    favoriteLocationsStorage_.readFromSettings();
    allLocations_ = new AllLocationsModel(this);
    configuredLocations_ = new ConfiguredCitiesModel(this);
    staticIpsLocations_ = new StaticIpsCitiesModel(this);
    favoriteLocations_ = new FavoriteCitiesModel(this);
}

LocationsModel::~LocationsModel()
{
    favoriteLocationsStorage_.writeToSettings();
}

void LocationsModel::updateApiLocations(const ProtoTypes::LocationId &bestLocation, const QString &staticIpDeviceName,
                                        const ProtoTypes::ArrayLocations &locations)
{
    apiLocations_.clear();

    bool isBestLocationInserted = false;
    bestLocationId_ = LocationID::createFromProtoBuf(bestLocation);
    numStaticIPLocations_ = numStaticIPLocationCities_ = 0;

    int cnt = locations.locations_size();
    for (int i = 0; i < cnt; ++i)
    {
        const ProtoTypes::Location &location = locations.locations(i);

        QSharedPointer<LocationModelItem> lmi(new LocationModelItem());
        lmi->initialInd_ = i;
        lmi->id = LocationID::createFromProtoBuf(location.id());
        lmi->title = QString::fromStdString(location.name());
        lmi->isShowP2P = location.is_p2p_supported();
        lmi->countryCode = QString::fromStdString(location.country_code()).toLower();
        lmi->isPremiumOnly = location.is_premium_only();
        lmi->is10gbps = false;

        qreal locationLoadSum = 0.0;
        int locationLoadCount = 0;

        for (int c = 0; c < location.cities_size(); ++c)
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
            cmi.linkSpeed = city.link_speed();

            // Engine is using -1 to indicate to us that the load (health) value was invalid/missing,
            // and therefore this location should be excluded when calculating the region's average
            // load value.
            cmi.locationLoad = city.health();
            if (cmi.locationLoad >= 0 && cmi.locationLoad <= 100)
            {
                locationLoadSum += cmi.locationLoad;
                locationLoadCount += 1;
            }
            else {
                cmi.locationLoad = 0;
            }

            lmi->cities << cmi;

            // if this is the best location then insert it to top list
            if (!isBestLocationInserted && !lmi->id.isStaticIpsLocation() && !lmi->id.isCustomConfigsLocation() && cmi.id.apiLocationToBestLocation() == bestLocationId_)
            {
                QSharedPointer<LocationModelItem> lmiBestLocation(new LocationModelItem());
                lmiBestLocation->initialInd_ = lmi->initialInd_;
                lmiBestLocation->id = bestLocationId_;
                lmiBestLocation->title = tr("Best Location");
                lmiBestLocation->countryCode = lmi->countryCode;
                lmiBestLocation->isShowP2P = lmi->isShowP2P;
                lmiBestLocation->isPremiumOnly = lmi->isPremiumOnly;
                lmiBestLocation->is10gbps = (city.link_speed() == 10000);
                lmiBestLocation->locationLoad = cmi.locationLoad;

                apiLocations_.insert(0, lmiBestLocation);
                isBestLocationInserted = true;
            }
        }

        if (locationLoadCount > 0) {
            lmi->locationLoad = qRound(locationLoadSum / locationLoadCount);
        }
        else {
            lmi->locationLoad = 0;
        }

        if (lmi->id.isStaticIpsLocation()) {
            ++numStaticIPLocations_;
            numStaticIPLocationCities_ += lmi->cities.count();
        }

        // sort cities alphabetically
        std::sort(lmi->cities.begin(), lmi->cities.end(), SortLocationsAlgorithms::lessThanByAlphabeticallyCityItem);

        apiLocations_ << lmi;

    }

    emit deviceNameChanged(staticIpDeviceName);


    allLocations_->update(apiLocations_);
    staticIpsLocations_->update(apiLocations_);
    favoriteLocations_->update(apiLocations_);

    emit bestLocationChanged(bestLocationId_);
}

void LocationsModel::updateBestLocation(const ProtoTypes::LocationId &bestLocation)
{
    LocationID bestLocationId = LocationID::createFromProtoBuf(bestLocation);
    if (bestLocationId == bestLocationId_)
    {
        return;
    }

    if (apiLocations_.count() > 0)
    {
        if (apiLocations_[0]->id.isBestLocation())
        {
            for (int i = 1; i < apiLocations_.count(); ++i)
            {
                int cities_cnt = apiLocations_[i]->cities.count();
                for (int c = 0; c < cities_cnt; ++c)
                {
                    if (!apiLocations_[i]->cities[c].id.isStaticIpsLocation() && apiLocations_[i]->cities[c].id.apiLocationToBestLocation() == bestLocationId)
                    {
                        apiLocations_[0]->id = bestLocationId;
                        apiLocations_[0]->countryCode = apiLocations_[i]->cities[c].countryCode;
                        apiLocations_[0]->isShowP2P = apiLocations_[i]->isShowP2P;
                        apiLocations_[0]->isPremiumOnly = apiLocations_[i]->isPremiumOnly;
                        apiLocations_[0]->is10gbps = apiLocations_[i]->is10gbps;
                        apiLocations_[0]->locationLoad = apiLocations_[i]->locationLoad;

                        bestLocationId_ = bestLocationId;
                        allLocations_->update(apiLocations_);
                        staticIpsLocations_->update(apiLocations_);
                        favoriteLocations_->update(apiLocations_);

                        emit bestLocationChanged(bestLocationId_);
                        return;
                    }
                }
            }
        }
        else
        {
            Q_ASSERT(false);
        }
    }
}

void LocationsModel::updateCustomConfigLocations(const ProtoTypes::ArrayLocations &locations)
{
    customConfigLocations_.clear();

    int cnt = locations.locations_size();
    for (int i = 0; i < cnt; ++i)
    {
        const ProtoTypes::Location &location = locations.locations(i);

        QSharedPointer<LocationModelItem> lmi(new LocationModelItem());
        lmi->initialInd_ = i;
        lmi->id = LocationID::createFromProtoBuf(location.id());
        lmi->title = QString::fromStdString(location.name());
        lmi->isShowP2P = location.is_p2p_supported();
        lmi->countryCode = QString::fromStdString(location.country_code()).toLower();
        lmi->isPremiumOnly = location.is_premium_only();
        lmi->is10gbps = false;
        lmi->locationLoad = 0;

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
            cmi.isCustomConfigCorrect = city.custom_config_is_correct();
            switch (city.custom_config_type()) {
            case ProtoTypes::CustomConfigType::CUSTOM_CONFIG_OPENVPN:
                cmi.customConfigType = "ovpn";
                break;
            case ProtoTypes::CustomConfigType::CUSTOM_CONFIG_WIREGUARD:
                cmi.customConfigType = "wg";
                break;
            default:
                Q_ASSERT(false);
                break;
            }
            cmi.customConfigErrorMessage = QString::fromStdString(city.custom_config_error_message());
            cmi.linkSpeed = 0;
            cmi.locationLoad = 0;
            lmi->cities << cmi;
        }

        // sort cities alphabetically
        std::sort(lmi->cities.begin(), lmi->cities.end(), SortLocationsAlgorithms::lessThanByAlphabeticallyCityItem);

        customConfigLocations_ << lmi;
    }

    configuredLocations_->update(customConfigLocations_);
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

void LocationsModel::saveFavorites()
{
    favoriteLocationsStorage_.writeToSettings();
}

bool LocationsModel::getLocationInfo(const LocationID &id, LocationsModel::LocationInfo &li)
{
    if (!id.isValid())
        return false;

    if (id.isCustomConfigsLocation())
    {
        for (int i = 0; i < customConfigLocations_.count(); ++i)
        {
            for (int k = 0; k < customConfigLocations_[i]->cities.count(); ++k)
            {
                if (customConfigLocations_[i]->cities[k].id == id)
                {
                    li.id = id;
                    li.firstName = customConfigLocations_[i]->cities[k].city;
                    li.secondName = customConfigLocations_[i]->cities[k].nick;
                    li.countryCode = customConfigLocations_[i]->cities[k].countryCode;
                    li.pingTime = customConfigLocations_[i]->cities[k].pingTimeMs;
                    return true;
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < apiLocations_.count(); ++i)
        {
            for (int k = 0; k < apiLocations_[i]->cities.count(); ++k)
            {
                const auto kIsStaticIp = apiLocations_[i]->cities[k].id.isStaticIpsLocation();
                if (id.isBestLocation() && !kIsStaticIp && !apiLocations_[i]->cities[k].id.isCustomConfigsLocation())
                {
                    if (apiLocations_[i]->cities[k].id.apiLocationToBestLocation() == id)
                    {
                        li.id = id;
                        li.firstName = tr("Best Location");
                        li.secondName = apiLocations_[i]->cities[k].city;
                        li.countryCode = apiLocations_[i]->cities[k].countryCode;
                        li.pingTime = apiLocations_[i]->cities[k].pingTimeMs;
                        return true;
                    }
                }
                else if (apiLocations_[i]->cities[k].id == id)
                {
                    li.id = id;
                    li.firstName = apiLocations_[i]->cities[k].city;
                    li.secondName = kIsStaticIp ? apiLocations_[i]->cities[k].staticIp
                                                : apiLocations_[i]->cities[k].nick;
                    li.countryCode = apiLocations_[i]->cities[k].countryCode;
                    li.pingTime = apiLocations_[i]->cities[k].pingTimeMs;
                    return true;
                }
            }
        }
    }

    return false;
}

QSharedPointer<LocationModelItem>
LocationsModel::getLocationModelItemByTitle(const QString &title) const
{
    for (int i = 0; i < apiLocations_.count(); ++i) {
        if (apiLocations_[i]->title == title)
            return apiLocations_[i];
    }
    return nullptr;
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
        for (auto lmi : apiLocations_) {
            for (auto &cmi : lmi->cities) {
                if (cmi.id == id) {
                    cmi.pingTimeMs = speed;
                    return;
                }
            }
        }
        for (auto lmi : customConfigLocations_) {
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

    // additionally emit signal if id is best location
    if (!id.isStaticIpsLocation() && !id.isCustomConfigsLocation())
    {
        if (id.apiLocationToBestLocation() == bestLocationId_)
        {
            emit locationSpeedChanged(bestLocationId_, speed);
        }
    }
}

LocationID LocationsModel::findLocationByFilter(const QString &strFilter) const
{
    QString strFilterLower = strFilter.toLower();
    auto getEnabledCities = [](QSharedPointer<LocationModelItem> l) {
        QList<CityModelItem> cities;
        for (auto it : qAsConst(l->cities))
        {
            if (!it.isDisabled)
            {
                cities << it;
            }
        }
        return cities;
    };

    // try find by region(location title) first
    auto lmiRegion = std::find_if(apiLocations_.begin(), apiLocations_.end(), [strFilterLower](QSharedPointer<LocationModelItem> item) {
            return !item->id.isStaticIpsLocation() && item->title.toLower() == strFilterLower;
    });

    if (lmiRegion != apiLocations_.end())
    {
        LocationID lid = (*lmiRegion)->id;
        QList<CityModelItem> cities = getEnabledCities(*lmiRegion);
        int ind = Utils::generateIntegerRandom(0, cities.length()-1);
        return cities[ind].id;
    }
    else
    {
        QVector< CityModelItem > citiesWithCC;
        for (auto itr = apiLocations_.begin(); itr != apiLocations_.end(); ++itr)
        {
            if (!(*itr)->id.isStaticIpsLocation() && (*itr)->countryCode.toLower() == strFilterLower)
            {
                for (auto city : qAsConst((*itr)->cities))
                {
                    if (!city.isDisabled)
                    {
                        citiesWithCC << city;
                    }
                }
            }
        }

        if (citiesWithCC.length() > 0) // connect by country
        {
            int ind = Utils::generateIntegerRandom(0, citiesWithCC.length()-1);
            return citiesWithCC[ind].id;
        }
        else // not region or country
        {
            QVector< CityModelItem > cmiFoundCity;
            QVector< CityModelItem > cmiFoundNickname;

            for (auto itr = apiLocations_.begin(); itr != apiLocations_.end(); ++itr)
            {
                if (!(*itr)->id.isStaticIpsLocation())
                {
                    for (auto city : qAsConst((*itr)->cities))
                    {
                        if (!city.isDisabled)
                        {
                            if (city.city.toLower() == strFilterLower)
                            {
                                cmiFoundCity << city;
                            }
                            if (city.nick.toLower() == strFilterLower)
                            {
                                cmiFoundNickname << city;
                            }
                        }
                    }
                }
            }

            if (!cmiFoundCity.isEmpty())
            {
                int ind = Utils::generateIntegerRandom(0, cmiFoundCity.length()-1);
                return cmiFoundCity[ind].id;
            }

            if (!cmiFoundNickname.isEmpty())
            {
                int ind = Utils::generateIntegerRandom(0, cmiFoundNickname.length()-1);
                return cmiFoundNickname[ind].id;
            }
        }
    }

    return  LocationID();
}

LocationID LocationsModel::getBestLocationId() const
{
    Q_ASSERT(bestLocationId_.isValid());
    return bestLocationId_;
}

LocationID LocationsModel::getFirstValidCustomConfigLocationId() const
{
    for (int i = 0; i < customConfigLocations_.count(); ++i)
    {
        for (int k = 0; k < customConfigLocations_[i]->cities.count(); ++k)
        {
            if (!customConfigLocations_[i]->cities[k].isDisabled &&
                customConfigLocations_[i]->cities[k].isCustomConfigCorrect)
                return customConfigLocations_[i]->cities[k].id;
        }
    }
    return LocationID();
}

LocationID LocationsModel::findGenericLocationByTitle(const QString &title) const
{
    for (const auto &location : apiLocations_) {
        for (const auto &city : location->cities) {
            if (city.makeTitle() == title)
                return city.id;
        }
    }
    return LocationID();
}

LocationID LocationsModel::findCustomConfigLocationByTitle(const QString &title) const
{
    for (const auto &location : customConfigLocations_) {
        for (const auto &city : qAsConst(location->cities)) {
            if (city.makeTitle() == title)
                return city.id;
        }
    }
    return LocationID();
}

int LocationsModel::getNumGenericLocations() const
{
    return apiLocations_.count() - numStaticIPLocations_;
}

int LocationsModel::getNumFavoriteLocations() const
{
    return favoriteLocationsStorage_.size();
}

int LocationsModel::getNumStaticIPLocations() const
{
    return numStaticIPLocationCities_;
}

int LocationsModel::getNumCustomConfigLocations() const
{
    return customConfigLocations_.count();
}
