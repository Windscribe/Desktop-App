#include "windflixlocationsmodel.h"

WindflixLocationsModel::WindflixLocationsModel(QObject *parent) : BasicLocationsModel(parent)
{

}

void WindflixLocationsModel::update(QVector<LocationModelItem *> locations)
{
    clearLocations();

    Q_FOREACH(LocationModelItem *lmi, locations)
    {
        // copy only WINDFLIX locations
        if (lmi->title.contains("WINDFLIX"))
        {
            LocationModelItem *nlmi = new LocationModelItem();
            *nlmi = *lmi;
            locations_ << nlmi;
        }
    }

    emit itemsUpdated(locations_);
}
