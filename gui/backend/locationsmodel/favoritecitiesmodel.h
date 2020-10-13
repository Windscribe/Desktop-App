#ifndef FAVORITECITIESMODEL_H
#define FAVORITECITIESMODEL_H

#include "basiccitiesmodel.h"

class FavoriteCitiesModel : public BasicCitiesModel
{
    Q_OBJECT
public:
    explicit FavoriteCitiesModel(QObject *parent = nullptr);

    void update(QVector<QSharedPointer<LocationModelItem> > locations) override;
    void setOrderLocationsType(ProtoTypes::OrderLocationType orderLocationsType) override;
    void setIsFavorite(const LocationID &id, bool isFavorite) override;
    void setFreeSessionStatus(bool isFreeSessionStatus) override;

private:
    QVector<CityModelItem *> favoriteCities_;

    void selectOnlyFavorite();
};

#endif // FAVORITECITIESMODEL_H
