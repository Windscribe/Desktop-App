#ifndef FAVORITELOCATIONSSTORAGE_H
#define FAVORITELOCATIONSSTORAGE_H

#include <QObject>
#include <QSet>
#include "../types/locationid.h"

class FavoriteLocationsStorage : public QObject
{
    Q_OBJECT
public:

    void addToFavorites(const LocationID &locationId);
    void removeFromFavorites(const LocationID &locationId);
    bool isFavorite(const LocationID &locationId);

    void readFromSettings();
    void writeToSettings();

private:
    QSet<LocationID> favoriteLocations_;
};

#endif // FAVORITELOCATIONSSTORAGE_H
