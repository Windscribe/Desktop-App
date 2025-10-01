#pragma once

#include <QVector>
#include "api_responses/location.h"

class ApiUtils
{
public:
    static void mergeWindflixLocations(QVector<api_responses::Location> &locations);

};


