#ifndef SORTLOCATIONSALGORITHMS_H
#define SORTLOCATIONSALGORITHMS_H

#include "locationmodelitem.h"

class SortLocationsAlgorithms
{
public:
    static bool lessThanByGeography(LocationModelItem *item1, LocationModelItem *item2);
    static bool lessThanByAlphabetically(LocationModelItem *item1, LocationModelItem *item2);
    static bool lessThanByLatency(LocationModelItem *item1, LocationModelItem *item2);
};

#endif // SORTLOCATIONSALGORITHMS_H
