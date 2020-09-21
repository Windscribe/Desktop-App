#include "locationid.h"

const int typeIdLocationId = qRegisterMetaType<LocationID>("LocationID");

LocationID::LocationID(int id, const QString &cityName) : locationId_(id), city_(cityName)
{
}

LocationID::LocationID(int id) : locationId_(id), city_()
{
}

LocationID LocationID::createFromApiLocation(int id, int cityId)
{
    return LocationID(id, "wind" + QString::number(cityId));
}

LocationID LocationID::createFromStaticIpsLocation(const QString &city, const QString &ip)
{
    return LocationID(STATIC_IPS_LOCATION_ID, city + "(" + ip + ")");
}

QString LocationID::getHashString() const
{
    Q_ASSERT(locationId_ != EMPTY_LOCATION);
    return QString::number(locationId_) + city_;
}
