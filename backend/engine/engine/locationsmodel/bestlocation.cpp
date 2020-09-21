#include "bestlocation.h"

namespace locationsmodel {

BestLocation::BestLocation() : isValid_(false)
{
}

bool BestLocation::isValid() const
{
    return isValid_;
}

int BestLocation::getId() const
{
    Q_ASSERT(isValid_);
    return id_;
}

void BestLocation::setId(int id)
{
    isValid_ = true;
    id_ = id;

}

} //namespace locationsmodel
