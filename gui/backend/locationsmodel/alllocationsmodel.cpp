#include "alllocationsmodel.h"


AllLocationsModel::AllLocationsModel(QObject *parent) : BasicLocationsModel(parent)
{
}

void AllLocationsModel::update(QVector<QSharedPointer<LocationModelItem> > locations)
{
    clearLocations();

    Q_FOREACH(QSharedPointer<LocationModelItem> lmi, locations)
    {
        if (lmi->id.isCustomConfigsLocation())
        {
            continue;
        }

        if (lmi->id.isStaticIpsLocation())
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
