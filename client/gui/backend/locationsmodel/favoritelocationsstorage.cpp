#include "favoritelocationsstorage.h"

#include <QDataStream>
#include <QSettings>

void FavoriteLocationsStorage::addToFavorites(const LocationID &locationId)
{
    if (!favoriteLocations_.contains(locationId))
    {
        favoriteLocations_.insert(locationId);
        isFavoriteLocationsSetModified_ = true;
    }
}

void FavoriteLocationsStorage::removeFromFavorites(const LocationID &locationId)
{
    if (favoriteLocations_.contains(locationId))
    {
        favoriteLocations_.remove(locationId);
        isFavoriteLocationsSetModified_ = true;
    }
}

bool FavoriteLocationsStorage::isFavorite(const LocationID &locationId)
{
    return favoriteLocations_.find(locationId) != favoriteLocations_.end();
}

int FavoriteLocationsStorage::size() const
{
    return favoriteLocations_.size();
}

void FavoriteLocationsStorage::readFromSettings()
{
    /*favoriteLocations_.clear();
    QSettings settings;

    if (settings.contains("favoriteLocations"))
    {
        QByteArray buf = settings.value("favoriteLocations").toByteArray();

        ProtoTypes::ArrayLocationId arrIds;
        if (arrIds.ParseFromArray(buf.data(), buf.size()))
        {
            for (int i = 0; i < arrIds.ids_size(); ++i)
            {
                favoriteLocations_.insert(LocationID::createFromProtoBuf(arrIds.ids(i)));
            }
        }
    }
    isFavoriteLocationsSetModified_ = false;*/
}

void FavoriteLocationsStorage::writeToSettings()
{
    /*if (!isFavoriteLocationsSetModified_)
        return;

    ProtoTypes::ArrayLocationId arrIds;
    for (const LocationID &lid : favoriteLocations_)
    {
        *arrIds.add_ids() = lid.toProtobuf();
    }

    size_t size = arrIds.ByteSizeLong();
    QByteArray arr(size, Qt::Uninitialized);
    arrIds.SerializeToArray(arr.data(), (int)size);

    QSettings settings;
    settings.setValue("favoriteLocations", arr);
    isFavoriteLocationsSetModified_ = false;*/
}
