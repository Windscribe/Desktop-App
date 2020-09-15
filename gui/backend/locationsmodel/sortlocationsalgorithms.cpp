#include "sortlocationsalgorithms.h"


bool SortLocationsAlgorithms::lessThanByGeography(LocationModelItem *item1, LocationModelItem *item2)
{
    if (item1->id.getId() == LocationID::BEST_LOCATION_ID && item2->id.getId() != LocationID::BEST_LOCATION_ID)
    {
        return true;
    }
    else if (item1->id.getId() != LocationID::BEST_LOCATION_ID && item2->id.getId() == LocationID::BEST_LOCATION_ID)
    {
        return false;
    }
    return item1->initialInd_ < item2->initialInd_;
}

bool SortLocationsAlgorithms::lessThanByAlphabetically(LocationModelItem *item1, LocationModelItem *item2)
{
    if (item1->id.getId() == LocationID::BEST_LOCATION_ID && item2->id.getId() != LocationID::BEST_LOCATION_ID)
    {
        return true;
    }
    else if (item1->id.getId() != LocationID::BEST_LOCATION_ID && item2->id.getId() == LocationID::BEST_LOCATION_ID)
    {
        return false;
    }
    return item1->title < item2->title;
}

bool SortLocationsAlgorithms::lessThanByLatency(LocationModelItem *item1, LocationModelItem *item2)
{
    if (item1->id.getId() == LocationID::BEST_LOCATION_ID && item2->id.getId() != LocationID::BEST_LOCATION_ID)
    {
        return true;
    }
    else if (item1->id.getId() != LocationID::BEST_LOCATION_ID && item2->id.getId() == LocationID::BEST_LOCATION_ID)
    {
        return false;
    }

    qint32 item1AverageLatency = item1->calcAveragePing();
    qint32 item2AverageLatency = item2->calcAveragePing();
    if (item1AverageLatency == item2AverageLatency)
    {
        return item1->title < item2->title;
    }
    else
    {
        return item1AverageLatency < item2AverageLatency;
    }
}
