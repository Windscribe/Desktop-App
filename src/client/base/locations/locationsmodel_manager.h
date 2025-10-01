#pragma once

#include <QTimer>

#include "model/locationsmodel.h"
#include "model/proxymodels/sortedlocations_proxymodel.h"
#include "model/proxymodels/sortedcities_proxymodel.h"

namespace gui_locations {

// Contains the model of locations and proxy models for various list views
class LocationsModelManager : public QObject
{
    Q_OBJECT
public:
    explicit LocationsModelManager(QObject *parent = nullptr);

    void updateLocations(const LocationID &bestLocation, const QVector<types::Location> &locations);
    void updateBestLocation(const LocationID &bestLocation);
    void updateCustomConfigLocation(const types::Location &location);
    void updateDeviceName(const QString &staticIpDeviceName);
    void changeConnectionSpeed(LocationID id, PingTime speed);
    void setLocationOrder(ORDER_LOCATION_TYPE orderLocationType);
    void setFreeSessionStatus(bool isFreeSessionStatus);

    QAbstractItemModel *locationsModel() { return locationsModel_; }
    QAbstractItemModel *sortedLocationsProxyModel() { return sortedLocationsProxyModel_; }
    QAbstractItemModel *filterLocationsProxyModel() { return filterLocationsProxyModel_; }
    QAbstractItemModel *favoriteCitiesProxyModel() { return favoriteCitiesProxyModel_; }
    QAbstractItemModel *staticIpsProxyModel() { return staticIpsProxyModel_; }
    QAbstractItemModel *customConfigsProxyModel() { return customConfigsProxyModel_; }

    QModelIndex getIndexByLocationId(const LocationID &id) const;
    // Could be region name, country code, server name
    // for example "Toronto", "The Six", "CA", "Canada East" would all be valid
    LocationID findLocationByFilter(const QString &strFilter) const;

    LocationID getBestLocationId() const;
    LocationID getFirstValidCustomConfigLocationId() const;

    // Sets filtering to filterLocationsProxyModel_
    void setFilterString(const QString &filterString);

    void saveFavoriteLocations();

    QJsonObject renamedLocations() const;
    void setRenamedLocations(const QJsonObject &obj);
    void resetRenamedLocations();

signals:
    void deviceNameChanged(const QString &deviceName);

private slots:
    void onChangeConnectionSpeedTimer();

private:
    LocationsModel *locationsModel_;
    SortedLocationsProxyModel *sortedLocationsProxyModel_;
    SortedLocationsProxyModel *filterLocationsProxyModel_;
    SortedCitiesProxyModel *sortedCitiesProxyModel_;
    QAbstractProxyModel *citiesProxyModel_;
    QAbstractProxyModel *favoriteCitiesProxyModel_;
    QAbstractProxyModel *staticIpsProxyModel_;
    QAbstractProxyModel *customConfigsProxyModel_;
    QString staticIpDeviceName_;

    const int UPDATE_CONNECTION_SPEED_PERIOD = 500;    // 0.5 sec
    QTimer timer_;
    QHash<LocationID, PingTime> connectionSpeeds_;
};

} //namespace gui_locations

