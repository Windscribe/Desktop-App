#include "customconfiglocationinfo.h"

namespace locationsmodel {

CustomConfigLocationInfo::CustomConfigLocationInfo(const LocationID &locationId, const QString &name, QSharedPointer<const customconfigs::ICustomConfig> config) :
    BaseLocationInfo(locationId, name), config_(config)
{

}


} //namespace locationsmodel
