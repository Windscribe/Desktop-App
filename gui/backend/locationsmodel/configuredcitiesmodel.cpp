#include "configuredcitiesmodel.h"

ConfiguredCitiesModel::ConfiguredCitiesModel(QObject *parent) : BasicCitiesModel(parent)
{

}

void ConfiguredCitiesModel::update(QVector<LocationModelItem *> locations)
{
    clearCities();

    for (const LocationModelItem *lmi : locations)
    {
        if (lmi->id.isCustomConfigsLocation())
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
