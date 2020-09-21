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

    void update(const ProtoTypes::ArrayLocations &arr);

    BasicLocationsModel *getAllLocationsModel();
    BasicCitiesModel *getConfiguredLocationsModel();
    BasicCitiesModel *getStaticIpsLocationsModel();
    BasicCitiesModel *getFavoriteLocationsModel();

    void setOrderLocationsType(ProtoTypes::OrderLocationType orderLocationsType);
    void switchFavorite(LocationID id, bool isFavorite);
    bool getLocationInfo(LocationID id, LocationInfo &li);

    QString countryCodeOfStaticCity(const QString &cityName);

    QList<CityModelItem> activeCityModelItems();
    QVector<LocationModelItem *> locationModelItems();

    void setFreeSessionStatus(bool isFreeSessionStatus);
    void changeConnectionSpeed(LocationID id, PingTime speed);

    LocationID getLocationIdByName(const QString &location) const;
    LocationID getLocationIdByCity(const QString &cityId, bool get_best_location) const;

signals:
    void locationSpeedChanged(LocationID id, PingTime speed);
    void deviceNameChanged(const QString &deviceName);

private:
    QString deviceName_;
    BasicLocationsModel *allLocations_;
    BasicCitiesModel *configuredLocations_;
    BasicCitiesModel *staticIpsLocations_;
    BasicCitiesModel *favoriteLocations_;
    FavoriteLocationsStorage favoriteLocationsStorage_;

    ProtoTypes::OrderLocationType orderLocationsType_;
    QVector<LocationModelItem *> locations_;

};

#endif // LOCATIONSMODEL_H
