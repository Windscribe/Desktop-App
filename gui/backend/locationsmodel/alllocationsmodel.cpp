#include "alllocationsmodel.h"


AllLocationsModel::AllLocationsModel(QObject *parent) : BasicLocationsModel(parent)
{
}

void AllLocationsModel::update(QVector<LocationModelItem *> locations)
{
    clearLocations();

    Q_FOREACH(LocationModelItem *lmi, locations)
    {
        // skip WINDFLIX locations
        if (lmi->title.contains("WINDFLIX"))
        {
            continue;
        }

        if (lmi->id.getId() == LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID)
        {
            continue;
        }

        if (lmi->id.getId() == LocationID::STATIC_IPS_LOCATION_ID)
        {
            continue;
        }

        LocationModelItem *nlmi = new LocationModelItem();
        *nlmi = *lmi;
        locations_ << nlmi;
    }


    sort();
    emit itemsUpdated(locations_);
}
