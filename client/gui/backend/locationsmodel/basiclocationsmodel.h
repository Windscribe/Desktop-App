#ifndef BASICLOCATIONSMODEL_H
#define BASICLOCATIONSMODEL_H

#include <QObject>
#include <QSharedPointer>
#include "locationmodelitem.h"
#include "types/enums.h"

class BasicLocationsModel : public QObject
{
    Q_OBJECT
public:
    explicit BasicLocationsModel(QObject *parent = nullptr);
    virtual ~BasicLocationsModel();

    virtual void update(QVector<QSharedPointer<LocationModelItem>> locations) = 0;

    void setOrderLocationsType(ORDER_LOCATION_TYPE orderLocationsType);
    void changeConnectionSpeed(LocationID id, PingTime speed);
    void setIsFavorite(LocationID id, bool isFavorite);
    void setFreeSessionStatus(bool isFreeSessionStatus);

    QVector<LocationModelItem *> items();

signals:
    void itemsUpdated(QVector<LocationModelItem*> items);  // only for direct connection
    void connectionSpeedChanged(LocationID id, PingTime timeMs);
    void isFavoriteChanged(LocationID id, bool isFavorite);
    void freeSessionStatusChanged(bool isFreeSessionStatus);

protected:
    QVector<LocationModelItem *> locations_;
    ORDER_LOCATION_TYPE orderLocationsType_;
    bool isFreeSessionStatus_;

    void clearLocations();
    void sort();
};

#endif // BASICLOCATIONSMODEL_H
