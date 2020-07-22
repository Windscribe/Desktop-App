#include "staticipscitiesmodel.h"

StaticIpsCitiesModel::StaticIpsCitiesModel(QObject *parent) : BasicCitiesModel(parent)
{

}

void StaticIpsCitiesModel::update(QVector<LocationModelItem *> locations)
{
    clearCities();

    Q_FOREACH(LocationModelItem *lmi, locations)
    {
        if (lmi->id.getId() == LocationID::STATIC_IPS_LOCATION_ID)
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
