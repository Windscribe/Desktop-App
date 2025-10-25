#pragma once

#include <QObject>
#include <QSet>
#include <QHash>
#include "types/locationid.h"

namespace gui_locations {

class FavoriteLocationsStorage : public QObject
{
    Q_OBJECT
public:

    void addToFavorites(const LocationID &locationId, const QString &hostname = QString(), const QString &ip = QString());
    void removeFromFavorites(const LocationID &locationId);
    bool isFavorite(const LocationID &locationId) const;
    QString getPinnedIp(const LocationID &locationId) const;
    QString getPinnedHostname(const LocationID &locationId) const;

    void readFromSettings();
    void writeToSettings();

private:
    struct FavoriteData {
        QString pinnedHostname;
        QString pinnedIp;

        friend QDataStream &operator<<(QDataStream &stream, const FavoriteData &data)
        {
            stream << data.pinnedHostname << data.pinnedIp;
            return stream;
        }

        friend QDataStream &operator>>(QDataStream &stream, FavoriteData &data)
        {
            stream >> data.pinnedHostname >> data.pinnedIp;
            return stream;
        }
    };

    QHash<LocationID, FavoriteData> favoriteLocations_;
    bool isFavoriteLocationsSetModified_ = false;

    static constexpr quint32 magic_ = 0xAA43C2AE;
    static constexpr int versionForSerialization_ = 2;  // should increment the version if the data format is changed
};

} //namespace gui_locations

