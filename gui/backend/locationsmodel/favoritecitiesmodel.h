#ifndef FAVORITECITIESMODEL_H
#define FAVORITECITIESMODEL_H

#include "basiccitiesmodel.h"

class FavoriteCitiesModel : public BasicCitiesModel
{
    Q_OBJECT
public:
    explicit FavoriteCitiesModel(QObject *parent = nullptr);

    virtual void update(QVector<LocationModelItem *> locations);
    virtual void setOrderLocationsType(ProtoTypes::OrderLocationType orderLocationsType);
    virtual void setIsFavorite(LocationID id, bool isFavorite);
    virtual void setFreeSessionStatus(bool isFreeSessionStatus);

private:
    QVector<CityModelItem *> favoriteCities_;

    void selectOnlyFavorite();
};

#endif // FAVORITECITIESMODEL_H
