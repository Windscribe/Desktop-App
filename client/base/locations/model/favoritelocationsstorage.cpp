#include "favoritelocationsstorage.h"

#include <QDataStream>
#include <QDataStream>
#include <QIODevice>
#include <QSettings>

#include "utils/simplecrypt.h"
#include "types/global_consts.h"

namespace gui_locations {

void FavoriteLocationsStorage::addToFavorites(const LocationID &locationId)
{
    if (!favoriteLocations_.contains(locationId))
    {
        favoriteLocations_.insert(locationId);
        isFavoriteLocationsSetModified_ = true;
        writeToSettings();
    }
}

void FavoriteLocationsStorage::removeFromFavorites(const LocationID &locationId)
{
    if (favoriteLocations_.contains(locationId))
    {
        favoriteLocations_.remove(locationId);
        isFavoriteLocationsSetModified_ = true;
        writeToSettings();
    }
}

bool FavoriteLocationsStorage::isFavorite(const LocationID &locationId) const
{
    return favoriteLocations_.find(locationId) != favoriteLocations_.end();
}

void FavoriteLocationsStorage::readFromSettings()
{
    favoriteLocations_.clear();
    QSettings settings;
    if (settings.contains("favoriteLocations"))
    {
        SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
        QString str = settings.value("favoriteLocations", "").toString();
        QByteArray arr = simpleCrypt.decryptToByteArray(str);

        QDataStream ds(&arr, QIODevice::ReadOnly);

        quint32 magic, version;
        ds >> magic;
        if (magic == magic_)
        {
            ds >> version;
            if (version <= versionForSerialization_)
            {
                ds >> favoriteLocations_;
                if (ds.status() != QDataStream::Ok)
                    favoriteLocations_.clear();
            }
        }
    }
    isFavoriteLocationsSetModified_ = false;
}

void FavoriteLocationsStorage::writeToSettings()
{
    if (!isFavoriteLocationsSetModified_)
        return;

    QByteArray arr;
    {
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << magic_;
        ds << versionForSerialization_;
        ds << favoriteLocations_;
    }
    QSettings settings;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    settings.setValue("favoriteLocations", simpleCrypt.encryptToString(arr));

    isFavoriteLocationsSetModified_ = false;
}

} //namespace gui_locations
