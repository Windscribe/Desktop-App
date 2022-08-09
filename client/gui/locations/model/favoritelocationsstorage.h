#ifndef GUI_LOCATIONS_FAVORITELOCATIONSSTORAGE_H
#define GUI_LOCATIONS_FAVORITELOCATIONSSTORAGE_H

#include <QObject>
#include <QSet>
#include "types/locationid.h"

namespace gui {

class FavoriteLocationsStorage : public QObject
{
    Q_OBJECT
public:

    void addToFavorites(const LocationID &locationId);
    void removeFromFavorites(const LocationID &locationId);
    bool isFavorite(const LocationID &locationId) const;

    void readFromSettings();
    void writeToSettings();

private:
    QSet<LocationID> favoriteLocations_;
    bool isFavoriteLocationsSetModified_ = false;

    static constexpr quint32 magic_ = 0xAA43C2AE;
    static constexpr int versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} //namespace gui

#endif // GUI_LOCATIONS_FAVORITELOCATIONSSTORAGE_H
