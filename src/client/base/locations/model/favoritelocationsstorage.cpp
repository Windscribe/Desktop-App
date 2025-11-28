#include "favoritelocationsstorage.h"

#include <QDataStream>
#include <QDataStream>
#include <QIODevice>
#include <QSettings>

#include "utils/simplecrypt.h"
#include "types/global_consts.h"

namespace gui_locations {

void FavoriteLocationsStorage::addToFavorites(const LocationID &locationId, const QString &hostname, const QString &ip)
{
    bool needsUpdate = false;

    if (!favoriteLocations_.contains(locationId)) {
        FavoriteData data;
        data.pinnedHostname = hostname;
        data.pinnedIp = ip;
        favoriteLocations_.insert(locationId, data);
        needsUpdate = true;
    } else {
        FavoriteData &data = favoriteLocations_[locationId];
        if (data.pinnedHostname != hostname || data.pinnedIp != ip) {
            data.pinnedHostname = hostname;
            data.pinnedIp = ip;
            needsUpdate = true;
        }
    }

    if (needsUpdate) {
        isFavoriteLocationsSetModified_ = true;
        writeToSettings();
    }
}

void FavoriteLocationsStorage::removeFromFavorites(const LocationID &locationId)
{
    if (favoriteLocations_.contains(locationId)) {
        favoriteLocations_.remove(locationId);
        isFavoriteLocationsSetModified_ = true;
        writeToSettings();
    }
}

bool FavoriteLocationsStorage::isFavorite(const LocationID &locationId) const
{
    return favoriteLocations_.contains(locationId);
}

QString FavoriteLocationsStorage::getPinnedIp(const LocationID &locationId) const
{
    auto it = favoriteLocations_.find(locationId);
    if (it != favoriteLocations_.end()) {
        return it.value().pinnedIp;
    }
    return QString();
}

QString FavoriteLocationsStorage::getPinnedHostname(const LocationID &locationId) const
{
    auto it = favoriteLocations_.find(locationId);
    if (it != favoriteLocations_.end()) {
        return it.value().pinnedHostname;
    }
    return QString();
}

void FavoriteLocationsStorage::readFromSettings()
{
    favoriteLocations_.clear();
    QSettings settings;
    if (settings.contains("favoriteLocations")) {
        SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
        QString str = settings.value("favoriteLocations", "").toString();
        QByteArray arr = simpleCrypt.decryptToByteArray(str);

        QDataStream ds(&arr, QIODevice::ReadOnly);

        quint32 magic, version;
        ds >> magic;
        if (magic == magic_) {
            ds >> version;
            if (version > versionForSerialization_) {
                qCDebug(LOG_BASIC) << "Detected future version of favorite locations storage, clearing data";
                favoriteLocations_.clear();
                writeToSettings();
                return;
            }

            if (version == 1) {
                // Migrate from version 1: QSet<LocationID> -> QHash<LocationID, FavoriteData>
                QSet<LocationID> oldFavorites;
                ds >> oldFavorites;
                if (ds.status() == QDataStream::Ok) {
                    for (const LocationID &locationId : oldFavorites) {
                        FavoriteData data;
                        data.pinnedHostname = QString();
                        data.pinnedIp = QString();
                        favoriteLocations_.insert(locationId, data);
                    }
                    isFavoriteLocationsSetModified_ = true;
                }
            } else if (version >= 2) {
                ds >> favoriteLocations_;
            }
        }
    }

    // If we have migrated to a new version, rewrite the data back to storage
    writeToSettings();
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
