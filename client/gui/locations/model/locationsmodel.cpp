#include "locationsmodel.h"
#include "../locationsmodel_roles.h"
#include <QDebug>

namespace gui_location {

LocationsModel::LocationsModel(QObject *parent) : QAbstractItemModel(parent), isFreeSessionStatus_(false)
{
    root_ = new int();
    favoriteLocationsStorage_.readFromSettings();
}

LocationsModel::~LocationsModel()
{
    favoriteLocationsStorage_.writeToSettings();
    clearLocations();
    delete root_;
}

void LocationsModel::updateLocations(const LocationID &bestLocation, const QVector<types::LocationItem> &locations)
{
    if (locations_.empty())
    {
        // just copy the list if the first update
        beginResetModel();
        int i = 0;
        for (const auto &l : locations)
        {
            LocationWrapper *li = new LocationWrapper(l, i);
            locations_ << li;
            mapLocations_[l.id] = li;
            i++;
        }
        updateBestLocation(bestLocation);
        endResetModel();
    }
    else
    {
        // compare lists and find new, updated and removed locations
        QVector<int> newLocationsInd;
        QVector<int> changedLocationsInd;
        QSet<LocationID> existsLocationsLID;

        for (int i = 0; i < locations.size(); ++i)
        {
            LocationID lid = locations[i].id;

            auto it = mapLocations_.find(lid);
            if (it == mapLocations_.end())
            {
                newLocationsInd << i;
            }
            else
            {
                LocationWrapper lw(locations[i], i);
                if (lw != *(it.value()))
                {
                    changedLocationsInd << i;
                }
                existsLocationsLID << lid;
            }
        }

        QVector<LocationID> removedLocationsLID;
        for (int i = 0; i < locations_.size(); ++i)
        {
            LocationID lid = locations_[i]->location().id;
            // the best location and custom configs location here is not taken into account
            if (lid.isBestLocation() || lid.isCustomConfigsLocation())
            {
                continue;
            }
            if (!existsLocationsLID.contains(lid))
            {
                removedLocationsLID << lid;
            }
        }

        // make changes for changed locations
        if (!changedLocationsInd.isEmpty())
        {
            for (auto locationInd : qAsConst(changedLocationsInd))
            {
                LocationID lid = locations[locationInd].id;

                auto it = mapLocations_.find(lid);
                Q_ASSERT(it != mapLocations_.end());
                if (it != mapLocations_.end())
                {
                    int ind = locations_.indexOf(it.value());
                    Q_ASSERT(ind != -1);
                    QModelIndex rootIndex = index(ind, 0);

                    QVector<int> newCitiesInd, removedCitiesInd;
                    QVector< QPair<int, int> > changedCities;
                    findCityChanges(it.value()->location(), locations[locationInd], newCitiesInd, removedCitiesInd, changedCities);

                    if (!changedCities.isEmpty())
                    {
                        for (auto i : changedCities)
                        {
                            it.value()->updateCity(i.first, locations[locationInd].cities[i.second]);

                            QModelIndex topLeftIndex = index(i.first, 0, rootIndex);
                            QModelIndex bottomRightIndex = index(i.first, 0, rootIndex);
                            emit dataChanged(topLeftIndex, bottomRightIndex);
                        }
                    }

                    if (!removedCitiesInd.isEmpty())
                    {
                        for (int i = removedCitiesInd.size() - 1; i >= 0; --i)
                        {
                            beginRemoveRows(rootIndex, removedCitiesInd[i], removedCitiesInd[i]);
                            it.value()->removeCity(removedCitiesInd[i]);
                            endRemoveRows();
                        }
                    }

                    if (!newCitiesInd.isEmpty())
                    {
                        beginInsertRows(rootIndex, it.value()->location().cities.size(), it.value()->location().cities.size() + newCitiesInd.size() - 1);
                        for (auto i : newCitiesInd)
                        {
                            it.value()->addCity(locations[locationInd].cities[i]);
                        }
                        endInsertRows();
                    }

                    if (ind != -1)
                    {
                        it.value()->updateLocationInfoOnly(locations[locationInd], locationInd);
                        emit dataChanged(rootIndex, rootIndex);
                    }
                }
            }
        }

        // make changes for new locations
        if (!newLocationsInd.isEmpty())
        {
            beginInsertRows(QModelIndex(), locations_.size(), locations_.size() + newLocationsInd.size() - 1);

            for (auto ind : qAsConst(newLocationsInd))
            {
                LocationWrapper *li = new LocationWrapper(locations[ind], ind);
                locations_ << li;
                mapLocations_[li->location().id] = li;
            }

            endInsertRows();
        }

        // make changes for removed locations
        if (!removedLocationsLID.isEmpty())
        {
            for (auto lid : qAsConst(removedLocationsLID))
            {
                auto it = locations_.begin();
                while (it != locations_.end())
                {
                    LocationID curLid = (*it)->location().id;
                    if (curLid == lid)
                    {
                        int indexOf = locations_.indexOf(*it);
                        beginRemoveRows(QModelIndex(), indexOf, indexOf);
                        mapLocations_.remove(lid);
                        delete (*it);
                        it = locations_.erase(it);
                        endRemoveRows();
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
        }

        // check for the best location change
        updateBestLocation(bestLocation);
    }
}

void LocationsModel::updateBestLocation(const LocationID &bestLocation)
{
    if (locations_.isEmpty()) {
        return;
    }
    LocationWrapper *blw = findAndCreateBestLocation(bestLocation);
    LocationID firstLocationId = locations_[0]->location().id;
    if (firstLocationId.isBestLocation())
    {
        if (blw)
        {
            // change the best location only if it has actually changed
            if (*blw != *locations_[0])
            {
                delete locations_[0];
                mapLocations_.remove(firstLocationId);
                locations_[0] = blw;
                mapLocations_[blw->location().id] = blw;

                emit dataChanged(index(0, 0), index(0, 0));
            }
            else
            {
                delete blw;
            }
        }
        else
        {
            // delete the best location (first item) because the best location is not found
            beginRemoveRows(QModelIndex(), 0, 0);
            mapLocations_.remove(firstLocationId);
            delete locations_[0];
            locations_.remove(0);
            endRemoveRows();
        }
    }
    else
    {
        // insert the best location to the top of the list
        if (blw)
        {
            beginInsertRows(QModelIndex(), 0, 0);
            locations_.insert(0, blw);
            LocationID bestLocationLid = blw->location().id;
            mapLocations_[bestLocationLid] = blw;
            endInsertRows();
        }
    }
}

void LocationsModel::updateCustomConfigLocation(const types::LocationItem &location)
{
    Q_ASSERT(location.id.isCustomConfigsLocation());

    // check if the custom location already inserted in the list
    LocationID lid = LocationID::createTopCustomConfigsLocationId();
    auto it = mapLocations_.find(lid);
    if (it != mapLocations_.end())
    {
        // update custom config location
        LocationWrapper new_cc(location, CUSTOM_CONFIG_INTERNAL_IND);
        if (new_cc != *it.value())
        {
            *it.value() = new_cc;

            int ind = locations_.indexOf(it.value());
            Q_ASSERT(ind != -1);

            QModelIndex topLeftIndex = index(ind, 0);
            emit dataChanged(topLeftIndex, topLeftIndex);
            int childCount = rowCount(topLeftIndex);
            QModelIndex topLeftChildIndex = index(0, 0, topLeftIndex);
            QModelIndex bottomRightChildIndex = index(childCount - 1, 0, topLeftIndex);
            emit dataChanged(topLeftChildIndex, bottomRightChildIndex);
        }
    }
    else
    {
        // Insert custom config location
        beginInsertRows(QModelIndex(), locations_.size(), locations_.size());
        LocationWrapper *li = new LocationWrapper(location, CUSTOM_CONFIG_INTERNAL_IND);
        locations_ << li;
        mapLocations_[lid] = li;
        endInsertRows();
    }
}

void LocationsModel::changeConnectionSpeed(LocationID id, PingTime speed)
{
    auto it = mapLocations_.find(id.toTopLevelLocation());
    if (it != mapLocations_.end())
    {
        int ind = locations_.indexOf(it.value());
        Q_ASSERT(ind != -1);
        if (ind != -1)
        {
            for (int c = 0; c < it.value()->location().cities.size(); ++c)
            {
                if (it.value()->location().cities[c].id == id)
                {
                    it.value()->setPingTimeForCity(c, speed.toInt());
                    it.value()->recalcAveragePing();
                    QModelIndex locationModelInd = index(ind, 0);
                    emit dataChanged(locationModelInd, locationModelInd);
                    QModelIndex cityModelInd = index(c, 0, locationModelInd);
                    emit dataChanged(cityModelInd, cityModelInd);
                    break;
                }
            }
        }
    }

    if (locations_.size() > 0)
    {
        // update speed for best location
        if (!id.isCustomConfigsLocation() && !id.isStaticIpsLocation() && locations_[0]->location().id == id.apiLocationToBestLocation())
        {
            locations_[0]->setAveragePing(speed.toInt());
            emit dataChanged(index(0, 0), index(0, 0));
        }
    }
}

void LocationsModel::setFreeSessionStatus(bool isFreeSessionStatus)
{
    if (isFreeSessionStatus != isFreeSessionStatus_)
    {
        isFreeSessionStatus_ = isFreeSessionStatus;
        emit dataChanged(index(0, 0), index(locations_.size() - 1, 0));
        for (int i = 0; i < locations_.size(); ++i)
        {
            if (locations_[i]->location().cities.size() > 0)
            {
                QModelIndex parentInd = index(i, 0);
                emit dataChanged(index(0, 0, parentInd), index(locations_[i]->location().cities.size() - 1, 0, parentInd));
            }
        }
    }
}

int LocationsModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 1;
}

QVariant LocationsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    if (index.internalPointer() == (void *)root_)
    {
        return dataForLocation(index.row(), role);
    }
    else
    {
        return dataForCity((LocationWrapper *)index.internalPointer(), index.row(), role);
    }
}

QModelIndex LocationsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
    {
        return QModelIndex();
    }

    if (!parent.isValid())
    {
        if (row >= 0 && row < locations_.size() && column == 0)
        {
            return createIndex(row, column, (void *)root_);
        }
    }
    else
    {
        Q_ASSERT(parent.internalPointer() != nullptr);
        if ((int *)parent.internalPointer() == root_ && parent.row() >= 0 && parent.row() < locations_.size())
        {
            LocationWrapper *li = locations_[parent.row()];
            if (row >= 0 && row < li->location().cities.size()  && column == 0)
            {
                return createIndex(row, column, (void *)li);
            }
        }
    }

    return QModelIndex();
}

QModelIndex LocationsModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return QModelIndex();
    }
    if (index.internalPointer() == (void *)root_)
    {
        return QModelIndex();
    }

    LocationWrapper *li = (LocationWrapper *)index.internalPointer();
    int ind = locations_.indexOf(li);
    Q_ASSERT(ind != -1);
    if (ind != -1)
    {
        return createIndex(ind, 0, (void *)root_);
    }

    return QModelIndex();
}

int LocationsModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        return locations_.size();
    }
    else
    {
        if (parent.internalPointer() == root_)
        {
            return locations_[parent.row()]->location().cities.size();
        }
    }

    return 0;
}

Qt::ItemFlags LocationsModel::flags(const QModelIndex &index) const
{
    if (!index.parent().isValid())
    {
        return Qt::NoItemFlags;
    }
    else
    {
        return Qt::ItemNeverHasChildren;
    }
}

bool LocationsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == IS_FAVORITE)
    {
        if (!index.isValid())
        {
            return false;
        }
        // can't set favorite for country
        if (index.internalPointer() == (void *)root_)
        {
            return false;
        }
        else
        {
            LocationWrapper *lw = (LocationWrapper *)index.internalPointer();
            LocationID lid = lw->location().cities[index.row()].id;
            if (value.toBool() == true)
            {
                favoriteLocationsStorage_.addToFavorites(lid);
            }
            else
            {
                favoriteLocationsStorage_.removeFromFavorites(lid);
            }
            emit dataChanged(index, index);
            return true;
        }
    }
    return false;
}

QModelIndex LocationsModel::getIndexByLocationId(const LocationID &id) const
{
    if (id.isBestLocation())
    {
        auto it = mapLocations_.find(id);
        if (it != mapLocations_.end())
        {
            int ind = locations_.indexOf(it.value());
            return index (ind, 0);
        }
    }
    else
    {
        auto it = mapLocations_.find(id.toTopLevelLocation());
        if (it != mapLocations_.end())
        {
            int ind = locations_.indexOf(it.value());

            if (id.isTopLevelLocation())
            {
                return index (ind, 0);
            }
            else
            {
                LocationWrapper *lw = it.value();
                for (int c = 0; c < lw->location().cities.size(); ++c)
                {
                    if (lw->location().cities[c].id == id)
                    {
                        QModelIndex locationModelInd = index(ind, 0);
                        QModelIndex cityModelInd = index(c, 0, locationModelInd);
                        return cityModelInd;
                    }
                }
            }
        }
    }

    return QModelIndex();
}

QModelIndex LocationsModel::getBestLocationIndex() const
{
    for (QHash<LocationID, LocationWrapper *>::const_iterator it = mapLocations_.constBegin(); it != mapLocations_.constEnd(); ++it)
    {
        if (it.key().isBestLocation())
        {
            int ind = locations_.indexOf(it.value());
            return index (ind, 0);
        }
    }
    return QModelIndex();
}

QModelIndex LocationsModel::getIndexByFilter(const QString &strFilter) const
{
    //todo
    Q_ASSERT(false);
    return QModelIndex();
}

QVariant LocationsModel::dataForLocation(int row, int role) const
{
    if (role == Qt::DisplayRole)
    {
        return locations_[row]->location().name + " - " + locations_[row]->location().countryCode + " - " + QString::number(locations_[row]->averagePing()) + " - " + QString::number(isFreeSessionStatus_);
    }
    else if (role == NAME)
    {
        return locations_[row]->location().name;
    }
    else if (role == NICKNAME)
    {
        return locations_[row]->nickname();
    }
    else if (role == INITITAL_INDEX)
    {
        return locations_[row]->initialInd();
    }
    else if (role == LOCATION_ID)
    {
        QVariant stored;
        stored.setValue(locations_[row]->location().id);
        return stored;
    }
    else if (role == IS_TOP_LEVEL_LOCATION)
    {
        return true;
    }
    else if (role == COUNTRY_CODE)
    {
        return locations_[row]->location().countryCode.toLower();
    }
    else if (role == IS_SHOW_P2P)
    {
        return locations_[row]->location().isNoP2P;
    }
    else if (role == IS_SHOW_AS_PREMIUM)
    {
        return locations_[row]->location().isPremiumOnly && isFreeSessionStatus_;
    }
    else if (role == IS_10GBPS)
    {
        return locations_[row]->is10gbps();
    }
    else if (role == LOAD)
    {
        return locations_[row]->load();
    }
    else if (role == PING_TIME)
    {
        return locations_[row]->averagePing();
    }

    return QVariant();
}

QVariant LocationsModel::dataForCity(LocationWrapper *l, int row, int role) const
{
    if (role == Qt::DisplayRole)
    {
        LocationID lid = l->location().cities[row].id;
        bool bFavorite = favoriteLocationsStorage_.isFavorite(lid);
        return l->location().cities[row].city + " - " + l->location().cities[row].nick + " - " + QString::number(l->location().cities[row].pingTimeMs.toInt()) + " - " + QString::number(bFavorite);
    }
    else if (role == INITITAL_INDEX)
    {
        return row;
    }
    else if (role == LOCATION_ID)
    {
        QVariant stored;
        stored.setValue(l->location().cities[row].id);
        return stored;
    }
    else if (role == IS_TOP_LEVEL_LOCATION)
    {
        return false;
    }
    else if (role == COUNTRY_CODE)
    {
        LocationID lid = l->location().cities[row].id;
        if (lid.isStaticIpsLocation())
        {
            return l->location().cities[row].staticIpCountryCode.toLower();
        }
        else
        {
            return l->location().countryCode.toLower();
        }
    }
    else if (role == IS_10GBPS)
    {
        return (l->location().cities[row].linkSpeed == 10000);
    }
    else if (role == LOAD)
    {
        // Engine is using -1 to indicate to us that the load (health) value was invalid/missing
        int cityHealth = l->location().cities[row].health;
        if (cityHealth >= 0 && cityHealth <= 100)
        {
            return cityHealth;
        }
        else
        {
            return 0;
        }
    }
    else if (role == NAME)
    {
        return l->location().cities[row].city;
    }
    else if (role == NICKNAME)
    {
        LocationID lid = l->location().cities[row].id;
        if (lid.isStaticIpsLocation())
        {
            return l->location().cities[row].staticIp;
        }
        else
        {
            return l->location().cities[row].nick;
        }
    }
    else if (role == PING_TIME)
    {
        return l->location().cities[row].pingTimeMs.toInt();
    }
    else if (role == IS_SHOW_AS_PREMIUM)
    {
        return l->location().cities[row].isPro && isFreeSessionStatus_;
    }
    else if (role == IS_FAVORITE)
    {
        LocationID lid = l->location().cities[row].id;
        return favoriteLocationsStorage_.isFavorite(lid);
    }
    else if (role == STATIC_IP_TYPE)
    {
        return l->location().cities[row].staticIpType;
    }
    else if (role == STATIC_IP)
    {
        return l->location().cities[row].staticIp;
    }
    else if (role == IS_DISABLED)
    {
        return l->location().cities[row].isDisabled;
    }
    else if (role == LINK_SPEED)
    {
        return l->location().cities[row].linkSpeed;
    }
    else if (role == IS_CUSTOM_CONFIG_CORRECT)
    {
        return l->location().cities[row].customConfigIsCorrect;
    }
    else if (role == CUSTOM_CONFIG_TYPE)
    {
        if (l->location().cities[row].customConfigType == CUSTOM_CONFIG_OPENVPN)
        {
            return "ovpn";
        }
        else if (l->location().cities[row].customConfigType == CUSTOM_CONFIG_WIREGUARD)
        {
            return "wg";
        }
        else
        {
            Q_ASSERT(false);
        }
    }
    else if (role == CUSTOM_CONFIG_ERROR_MESSAGE)
    {
        return l->location().cities[row].customConfigErrorMessage;
    }

    return QVariant();
}

void LocationsModel::clearLocations()
{
    for (auto it : qAsConst(locations_))
    {
        delete it;
    }
    locations_.clear();
    mapLocations_.clear();
}

LocationsModel::LocationWrapper *LocationsModel::findAndCreateBestLocation(const LocationID &bestLocation)
{
    if (!bestLocation.isValid()) {
        return nullptr;
    }

    auto it = mapLocations_.find(bestLocation.bestLocationToApiLocation().toTopLevelLocation());
    if (it != mapLocations_.end())
    {
        LocationWrapper *lw = it.value();
        for (int c = 0; c < lw->location().cities.size(); ++c)
        {
            LocationID cityLid = lw->location().cities[c].id;
            if (bestLocation == cityLid.apiLocationToBestLocation())
            {
                LocationWrapper *blw = new LocationWrapper(bestLocation, lw->location(), c);
                return blw;
            }
        }
    }

    return nullptr;
}

void LocationsModel::findCityChanges(const types::LocationItem &l1, const types::LocationItem &l2, QVector<int> &newCitiesInd,
                                     QVector<int> &removedCitiesInd, QVector<QPair<int, int> > &changedCities)
{
    // todo: I think it can be optimized together with the search for changes in locations
    QSet<int> foundedIndexes;

    for (int i1 = 0; i1 < l1.cities.size(); ++i1)
    {
        bool bFound = false;
        int foundInd;
        for (int i2 = 0; i2 < l2.cities.size(); ++i2)
        {
            if (l1.cities[i1].id == l2.cities[i2].id)
            {
                bFound = true;
                foundInd = i2;
                foundedIndexes.insert(i2);
                break;
            }
        }

        if (bFound)
        {
            if (l1.cities[i1] != l2.cities[foundInd])
            {
                changedCities << qMakePair(i1, foundInd);
            }
        }
        else
        {
            removedCitiesInd << i1;
        }
    }

    for (int i2 = 0; i2 < l2.cities.size(); ++i2)
    {
        if (!foundedIndexes.contains(i2))
        {
            newCitiesInd << i2;
        }
    }
}

} //namespace gui_location
