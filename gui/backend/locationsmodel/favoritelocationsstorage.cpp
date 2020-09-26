#include "favoritelocationsstorage.h"

#include <QDataStream>
#include <QSettings>

void FavoriteLocationsStorage::addToFavorites(const LocationID &locationId)
{
    if (!favoriteLocations_.contains(locationId))
    {
        favoriteLocations_.insert(locationId);
    }
}

void FavoriteLocationsStorage::removeFromFavorites(const LocationID &locationId)
{
    if (favoriteLocations_.contains(locationId))
    {
        favoriteLocations_.remove(locationId);
    }
}

bool FavoriteLocationsStorage::isFavorite(const LocationID &locationId)
{
    return favoriteLocations_.find(locationId) != favoriteLocations_.end();
}

void FavoriteLocationsStorage::readFromSettings()
{
    favoriteLocations_.clear();
    QSettings settings("Windscribe", "Windscribe");

    // remove old value from version 1
    /*if (settings.contains("favoriteLocations"))
    {
        settings.remove("favoriteLocations");
    }*/

    if (settings.contains("favoriteLocations2"))
    {
        QByteArray buf = settings.value("favoriteLocations2").toByteArray();

        ProtoTypes::ArrayLocationId arrIds;
        if (arrIds.ParseFromArray(buf.data(), buf.size()))
        {
            for (int i = 0; i < arrIds.ids_size(); ++i)
            {
                favoriteLocations_.insert(LocationID::createFromProtoBuf(arrIds.ids(i)));
            }
        }
    }
}

void FavoriteLocationsStorage::writeToSettings()
{
    ProtoTypes::ArrayLocationId arrIds;
    for (const LocationID &lid : favoriteLocations_)
    {
        *arrIds.add_ids() = lid.toProtobuf();
    }

    size_t size = arrIds.ByteSizeLong();
    QByteArray arr(size, Qt::Uninitialized);
    arrIds.SerializeToArray(arr.data(), size);

    QSettings settings("Windscribe", "Windscribe");
    settings.setValue("favoriteLocations2", arr);
}
