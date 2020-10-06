#include "baselocationinfo.h"

namespace locationsmodel {

BaseLocationInfo::BaseLocationInfo(const LocationID &locationId, const QString &name) : QObject(nullptr),
    locationId_(locationId), name_(name)
{
}

const LocationID &BaseLocationInfo::locationId() const
{
    return locationId_;
}

QString BaseLocationInfo::getName() const
{
    return name_;
}


} //namespace locationsmodel
