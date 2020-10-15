#ifndef LOCATIONSMODEL_H
#define LOCATIONSMODEL_H

#include <QObject>
#include "basiclocationsmodel.h"
#include "basiccitiesmodel.h"
#include "favoritelocationsstorage.h"
#include "ipc/protobufcommand.h"
#include "ipc/generated_proto/types.pb.h"

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

    void updateApiLocations(const ProtoTypes::LocationId &bestLocation, const ProtoTypes::ArrayLocations &locations);
    void updateBestLocation(const ProtoTypes::LocationId &bestLocation);
    void updateCustomConfigLocations(const ProtoTypes::ArrayLocations &locations);

    BasicLocationsModel *getAllLocationsModel();
    BasicCitiesModel *getConfiguredLocationsModel();
    BasicCitiesModel *getStaticIpsLocationsModel();
    BasicCitiesModel *getFavoriteLocationsModel();

    void setOrderLocationsType(ProtoTypes::OrderLocationType orderLocationsType);
    void switchFavorite(const LocationID &id, bool isFavorite);
    bool getLocationInfo(const LocationID &id, LocationInfo &li);

    //QList<CityModelItem> activeCityModelItems();
    //QVector<LocationModelItem *> locationModelItems();

    void setFreeSessionStatus(bool isFreeSessionStatus);
    void changeConnectionSpeed(LocationID id, PingTime speed);

    LocationID getLocationIdByName(const QString &location) const;

    // Could be region name, country code, server name
    // for example "Toronto", "The Six", "CA", "Canada East" would all be valid
    LocationID findLocationByFilter(const QString &strFilter) const;

    LocationID getBestLocationId() const;

signals:
    void locationSpeedChanged(LocationID id, PingTime speed);
    void bestLocationChanged(LocationID id);

    void deviceNameChanged(const QString &deviceName);

private:
    QString deviceName_;
    BasicLocationsModel *allLocations_;
    BasicCitiesModel *configuredLocations_;
    BasicCitiesModel *staticIpsLocations_;
    BasicCitiesModel *favoriteLocations_;
    FavoriteLocationsStorage favoriteLocationsStorage_;

    ProtoTypes::OrderLocationType orderLocationsType_;
    QVector< QSharedPointer<LocationModelItem> > apiLocations_;
    LocationID bestLocationId_;
    QVector< QSharedPointer<LocationModelItem> > customConfigLocations_;

};

#endif // LOCATIONSMODEL_H
