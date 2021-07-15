#ifndef BASICCITIESMODEL_H
#define BASICCITIESMODEL_H

#include <QObject>
#include <QSharedPointer>
#include "locationmodelitem.h"
#include "types/locationid.h"

class BasicCitiesModel : public QObject
{
    Q_OBJECT
public:
    explicit BasicCitiesModel(QObject *parent = nullptr);
    virtual ~BasicCitiesModel();

    virtual void update(QVector<QSharedPointer<LocationModelItem> > locations) = 0;

    virtual void setOrderLocationsType(ProtoTypes::OrderLocationType orderLocationsType);
    void changeConnectionSpeed(const LocationID &id, const PingTime &speed);
    virtual void setIsFavorite(const LocationID &id, bool isFavorite);
    virtual void setFreeSessionStatus(bool isFreeSessionStatus);

    QVector<CityModelItem *> getCities();

signals:
    void itemsUpdated(QVector<CityModelItem*> items);  // only for direct connection
    void connectionSpeedChanged(const LocationID &id, const PingTime &timeMs);
    void isFavoriteChanged(const LocationID &id, bool isFavorite);
    void freeSessionStatusChanged(bool isFreeSessionStatus);


protected:
    QVector<CityModelItem *> cities_;
    ProtoTypes::OrderLocationType orderLocationsType_;
    bool isFreeSessionStatus_;

    void clearCities();
    virtual void sort(QVector<CityModelItem *> &cities);
};

#endif // BASICCITIESMODEL_H
