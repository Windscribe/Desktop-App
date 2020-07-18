#include "locationid.h"

const int typeIdLocationId = qRegisterMetaType<LocationID>("LocationID");

LocationID::LocationID(int id, const QString &cityName)
{
    locationId_ = id;
    city_ = cityName;
}

LocationID::LocationID(int id)
{
    locationId_ = id;
    city_.clear();
}

QString LocationID::getHashString() const
{
    Q_ASSERT(locationId_ != EMPTY_LOCATION);
    return QString::number(locationId_) + city_;
}
