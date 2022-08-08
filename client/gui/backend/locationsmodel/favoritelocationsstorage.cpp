#include "favoritelocationsstorage.h"

#include <QDataStream>
#include <QJsonArray>
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
    favoriteLocations_.clear();
    QSettings settings;

    if (settings.contains("favoriteLocations"))
    {
        QByteArray buf = settings.value("favoriteLocations").toByteArray();
        QCborValue cbor = QCborValue::fromCbor(buf);
        if (!cbor.isInvalid() && cbor.isMap())
        {
            const QJsonObject &obj = cbor.toJsonValue().toObject();
            if (obj.contains("version") && obj["version"].toInt(INT_MAX) <= versionForSerialization_)
            {
                if (obj.contains("locations") && obj["locations"].isArray())
                {
                    QJsonArray arr = obj["locations"].toArray();
                    for (const auto &it: arr)
                    {
                        LocationID lid;
                        if (lid.fromJsonObject(it.toObject()))
                        {
                            favoriteLocations_.insert(lid);
                        }
                    }
                }
            }
        }
    }
    isFavoriteLocationsSetModified_ = false;
}

void FavoriteLocationsStorage::writeToSettings()
{
    if (!isFavoriteLocationsSetModified_)
        return;

    QJsonObject json;
    json["version"] = versionForSerialization_;
    QJsonArray arrJson;
    for (const LocationID &lid : favoriteLocations_)
    {
        arrJson << lid.toJsonObject();
    }
    json["locations"] = arrJson;

    QCborValue cbor = QCborValue::fromJsonValue(json);
    QByteArray arr = cbor.toCbor();

    QSettings settings;
    settings.setValue("favoriteLocations", arr);
    isFavoriteLocationsSetModified_ = false;
}
