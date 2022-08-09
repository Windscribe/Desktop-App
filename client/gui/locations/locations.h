#ifndef GUI_LOCATIONS_LOCATIONS_H
#define GUI_LOCATIONS_LOCATIONS_H

#include "model/locationsmodel.h"
#include "model/sortedlocationsmodel.h"
#include "model/citiesmodel.h"
#include "model/sortedcitiesmodel.h"
#include "model/favoritecitiesmodel.h"
#include "model/staticipsmodel.h"
#include "model/customconfigsmodel.h"

#include <QTimer>

namespace gui {

// Contains the model of locations and proxy models for various list views
class Locations : public QObject
{
    Q_OBJECT
public:
    explicit Locations(QObject *parent = nullptr);

    void updateLocations(const LocationID &bestLocation, const QVector<types::LocationItem> &locations);
    void updateDeviceName(const QString &staticIpDeviceName);
    void updateBestLocation(const LocationID &bestLocation);
    void updateCustomConfigLocation(const types::LocationItem &location);
    void changeConnectionSpeed(LocationID id, PingTime speed);
    void setLocationOrder(ORDER_LOCATION_TYPE orderLocationType);
    void setFreeSessionStatus(bool isFreeSessionStatus);

    QAbstractItemModel *getLocationsModel() { return locationsModel_; }
    QAbstractItemModel *getSortedLocationsModel() { return sortedLocationsModel_; }
    QAbstractItemModel *getFavoriteCitiesModel() { return favoriteCitiesModel_; }
    QAbstractItemModel *getStaticIpsModel() { return staticIpsModel_; }
    QAbstractItemModel *getCustomConfigsModel() { return customConfigsModel_; }

    QModelIndex getIndexByLocationId(const LocationID &id) const;
    // Could be region name, country code, server name
    // for example "Toronto", "The Six", "CA", "Canada East" would all be valid
    QModelIndex getIndexByFilter(const QString &strFilter) const;

    LocationID getBestLocationId() const;
    LocationID getFirstValidCustomConfigLocationId() const;
    LocationID findGenericLocationByTitle(const QString &title) const;
    LocationID findCustomConfigLocationByTitle(const QString &title) const;

    int getNumGenericLocations() const;
    int getNumFavoriteLocations() const;
    int getNumStaticIPLocations() const;
    int getNumCustomConfigLocations() const;

signals:
    void deviceNameChanged(const QString &deviceName);

private slots:
    void onChangeConnectionSpeedTimer();

private:
    LocationsModel *locationsModel_;
    SortedLocationsModel *sortedLocationsModel_;
    CitiesModel *citiesModel_;
    SortedCitiesModel *sortedCitiesModel_;
    FavoriteCitiesModel *favoriteCitiesModel_;
    StaticIpsModel *staticIpsModel_;
    CustomConfgisModel *customConfigsModel_;
    QString staticIpDeviceName_;

    const int UPDATE_CONNECTION_SPEED_PERIOD = 500;    // 0.5 sec
    QTimer timer_;
    QHash<LocationID, PingTime> connectionSpeeds_;
};

} //namespace gui


#endif // GUI_LOCATIONS_LOCATIONS_H
