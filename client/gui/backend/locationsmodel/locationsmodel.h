#ifndef LOCATIONSMODEL_H
#define LOCATIONSMODEL_H

#include <QObject>
#include "basiclocationsmodel.h"
#include "basiccitiesmodel.h"
#include "favoritelocationsstorage.h"
#include "types/locationitem.h"
#include "ipc/protobufcommand.h"

class LocationsModel : public QObject
{
    Q_OBJECT
public:

    struct LocationInfo
    {
        LocationID id;
        QString firstName;
        QString secondName;
        QString countryCode;
        PingTime pingTime;
    };

    explicit LocationsModel(QObject *parent = nullptr);
    virtual ~LocationsModel();

    void updateApiLocations(const LocationID &bestLocation, const QString &staticIpDeviceName,
                            const QVector<types::LocationItem> &locations);
    void updateBestLocation(const LocationID &bestLocation);
    void updateCustomConfigLocations(const QVector<types::LocationItem> &locations);

    BasicLocationsModel *getAllLocationsModel();
    BasicCitiesModel *getConfiguredLocationsModel();
    BasicCitiesModel *getStaticIpsLocationsModel();
    BasicCitiesModel *getFavoriteLocationsModel();

    void setOrderLocationsType(ORDER_LOCATION_TYPE orderLocationsType);
    void switchFavorite(const LocationID &id, bool isFavorite);
    void saveFavorites();
    bool getLocationInfo(const LocationID &id, LocationInfo &li);

    QSharedPointer<LocationModelItem> getLocationModelItemByTitle(const QString &title) const;

    void setFreeSessionStatus(bool isFreeSessionStatus);
    void changeConnectionSpeed(LocationID id, PingTime speed);

    //LocationID getLocationIdByName(const QString &location) const;

    // Could be region name, country code, server name
    // for example "Toronto", "The Six", "CA", "Canada East" would all be valid
    LocationID findLocationByFilter(const QString &strFilter) const;

    LocationID getBestLocationId() const;
    LocationID getFirstValidCustomConfigLocationId() const;
    LocationID findGenericLocationByTitle(const QString &title) const;
    LocationID findCustomConfigLocationByTitle(const QString &title) const;

    int getNumGenericLocations() const;
    int getNumFavoriteLocations() const;
    int getNumStaticIPLocations() const;
    int getNumCustomConfigLocations() const;

signals:
    void locationSpeedChanged(LocationID id, PingTime speed);
    void bestLocationChanged(LocationID id);

    void deviceNameChanged(const QString &deviceName);

private:
    BasicLocationsModel *allLocations_;
    BasicCitiesModel *configuredLocations_;
    BasicCitiesModel *staticIpsLocations_;
    BasicCitiesModel *favoriteLocations_;
    FavoriteLocationsStorage favoriteLocationsStorage_;

    ORDER_LOCATION_TYPE orderLocationsType_;
    QVector< QSharedPointer<LocationModelItem> > apiLocations_;
    LocationID bestLocationId_;
    QVector< QSharedPointer<LocationModelItem> > customConfigLocations_;
    int numStaticIPLocations_;
    int numStaticIPLocationCities_;
};

#endif // LOCATIONSMODEL_H
