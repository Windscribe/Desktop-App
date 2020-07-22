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
    if (settings.contains("favoriteLocations"))
    {
        settings.remove("favoriteLocations");
    }

    if (settings.contains("favoriteLocations2"))
    {
        QByteArray buf = settings.value("favoriteLocations2").toByteArray();
        QDataStream stream(&buf, QIODevice::ReadOnly);
        stream >> favoriteLocations_;
    }
}

void FavoriteLocationsStorage::writeToSettings()
{
    QByteArray buf;
    {
        QDataStream stream(&buf, QIODevice::WriteOnly);
        stream << favoriteLocations_;
    }
    QSettings settings("Windscribe", "Windscribe");
    settings.setValue("favoriteLocations2", buf);
}
