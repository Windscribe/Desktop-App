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

    if (item1->pingTimeMs.toInt() < 0 && item2->pingTimeMs.toInt() < 0)
    {
        return item1->title < item2->title;
    }
    else if (item1->pingTimeMs.toInt() < 0 && item2->pingTimeMs.toInt() >= 0)
    {
        return false;
    }
    else if (item1->pingTimeMs.toInt() >= 0 && item2->pingTimeMs.toInt() < 0)
    {
        return true;
    }
    else
    {
        if (item1->pingTimeMs.toInt() == item2->pingTimeMs.toInt())
        {
            return item1->title < item2->title;
        }
        else
        {
            return item1->pingTimeMs.toInt() < item2->pingTimeMs.toInt();
        }
    }
}
