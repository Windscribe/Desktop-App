#include "sortlocationsalgorithms.h"


bool SortLocationsAlgorithms::lessThanByGeography(LocationModelItem *item1, LocationModelItem *item2)
{
    if (item1->id.isBestLocation() && !item2->id.isBestLocation())
    {
        return true;
    }
    else if (!item1->id.isBestLocation() && item2->id.isBestLocation())
    {
        return false;
    }
    return item1->initialInd_ < item2->initialInd_;
}

bool SortLocationsAlgorithms::lessThanByAlphabetically(LocationModelItem *item1, LocationModelItem *item2)
{
    if (item1->id.isBestLocation() && !item2->id.isBestLocation())
    {
        return true;
    }
    else if (!item1->id.isBestLocation() && item2->id.isBestLocation())
    {
        return false;
    }
    return item1->title < item2->title;
}

bool SortLocationsAlgorithms::lessThanByLatency(LocationModelItem *item1, LocationModelItem *item2)
{
    if (item1->id.isBestLocation() && !item2->id.isBestLocation())
    {
        return true;
    }
    else if (!item1->id.isBestLocation() && item2->id.isBestLocation())
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
        if (item1AverageLatency == -1)
        {
            return false;
        }
        else if (item2AverageLatency == -1)
        {
            return true;
        }
        else
        {
            return item1AverageLatency < item2AverageLatency;
        }
    }
}

bool SortLocationsAlgorithms::lessThanByAlphabeticallyCityItem(const CityModelItem &item1, const CityModelItem &item2)
{
    return item1.makeTitle() < item2.makeTitle();
}
