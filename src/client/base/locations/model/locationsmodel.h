#pragma once

#include <QAbstractItemModel>
#include <QJsonObject>
#include "favoritelocationsstorage.h"
#include "types/location.h"
#include "types/locationid.h"
#include "types/pingtime.h"
#include "locationitem.h"
#include "renamedlocationsstorage.h"

namespace gui_locations {

/*  It is an implementation of the locations model based on QAbstractItemModel
    The structure is a simple unsorted tree. Only the best location is always added to the top of the list,
    other items can be in any order, proxy models are used for sorting.

     BestLocation (if exists)
     Country1
          City1
          ...
          CityN
     Country2
          City1
          ...
          CityN
          ...
     CountryN
     CustomConfig Location  (if exists)
          custom_config1
          ...
          custom_configN

    Here also management of the best location and CustomConfig location.
    The best location has no children
    Roles for LocationsModel items are located in locationsmodel_roles.h
    Can be used with any Qt-class based on QAbstractItemView (for example with QTreeView) to show this model in the GUI.
*/

class LocationsModel : public QAbstractItemModel
{
    Q_OBJECT
public:

    explicit LocationsModel(QObject *parent = nullptr);
    virtual ~LocationsModel();

    void updateLocations(const LocationID &bestLocation, const QVector<types::Location> &newLocations);
    void updateBestLocation(const LocationID &bestLocation);
    void updateCustomConfigLocation(const types::Location &location);
    void changeConnectionSpeed(LocationID id, PingTime speed);
    void setFreeSessionStatus(bool isFreeSessionStatus);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QModelIndex getIndexByLocationId(const LocationID &id) const;
    QModelIndex getBestLocationIndex() const;
    QModelIndex getCustomConfigLocationIndex() const;

    // the client of the class must explicitly save locations if required
    void saveFavoriteLocations();

    QJsonObject renamedLocations() const;
    void setRenamedLocations(const QJsonObject &obj);
    void resetRenamedLocations();

signals:
    void deviceNameChanged(const QString &deviceName);

private slots:
    void onLanguageChanged();

private:
    QVector<LocationItem *> locations_;
    QHash<LocationID, LocationItem *> mapLocations_;   // map LocationID to index in locations_

    int *root_;   // Fake root node. The typename does not matter, only the pointer to identify the root node matters.
    bool isFreeSessionStatus_;
    FavoriteLocationsStorage favoriteLocationsStorage_;
    RenamedLocationsStorage renamedLocationsStorage_;
    const char *BEST_LOCATION_NAME = QT_TR_NOOP("Best Location");

    QVariant dataForLocation(int row, int role) const;
    QVariant dataForCity(LocationItem *l, int row, int role) const;
    void clearLocations();
    void handleChangedLocation(int ind, const types::Location &newLocation);
    LocationItem *findAndCreateBestLocationItem(const LocationID &bestLocation, int *idNum, int *cityIdNum);

};

} //namespace gui_locations
