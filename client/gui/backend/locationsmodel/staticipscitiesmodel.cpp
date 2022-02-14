#include "staticipscitiesmodel.h"

StaticIpsCitiesModel::StaticIpsCitiesModel(QObject *parent) : BasicCitiesModel(parent)
{

}

void StaticIpsCitiesModel::update(QVector<QSharedPointer<LocationModelItem>> locations)
{
    clearCities();

    for (QSharedPointer<LocationModelItem> lmi : locations)
    {
        if (lmi->id.isStaticIpsLocation())
        {
            for (int i = 0; i < lmi->cities.count(); ++i)
            {
                CityModelItem *cmi = new CityModelItem();
                *cmi = lmi->cities[i];
                cities_ << cmi;
            }
        }
    }

    sort(cities_);
    emit itemsUpdated(cities_);
}
