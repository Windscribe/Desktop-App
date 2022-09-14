#include "locationid.h"
#include "utils/ws_assert.h"

const int typeIdLocationId = qRegisterMetaType<LocationID>("LocationID");

QString LocationID::getHashString() const
{
    WS_ASSERT(type_ != INVALID_LOCATION);
    return QString::number(id_) + QString::number(type_) + city_;
}

LocationID LocationID::createTopApiLocationId(int id)
{
    return LocationID(API_LOCATION, id, QString());
}

LocationID LocationID::createTopStaticLocationId()
{
    return LocationID(STATIC_IPS_LOCATION, 0, QString());
}

LocationID LocationID::createTopCustomConfigsLocationId()
{
    return LocationID(CUSTOM_CONFIGS_LOCATION, 0, QString());
}

LocationID LocationID::createApiLocationId(int id, const QString &city, const QString &nick)
{
    return LocationID(API_LOCATION, id, city + " - " + nick);
}

LocationID LocationID::createBestLocationId(int id)
{
    return LocationID(BEST_LOCATION, id, QString());
}

LocationID LocationID::createStaticIpsLocationId(const QString &city, const QString &ip)
{
    return LocationID(STATIC_IPS_LOCATION, 0, city + " - " + ip);
}

LocationID LocationID::createCustomConfigLocationId(const QString &filename)
{
    return LocationID(CUSTOM_CONFIGS_LOCATION, 0, filename);
}

bool LocationID::isTopLevelLocation() const
{
    WS_ASSERT(type_ != INVALID_LOCATION);
    return city_.isEmpty();
}

LocationID LocationID::bestLocationToApiLocation() const
{
    WS_ASSERT(type_ == BEST_LOCATION);
    return LocationID(API_LOCATION, id_, city_);
}

LocationID LocationID::apiLocationToBestLocation() const
{
    WS_ASSERT(type_ == API_LOCATION);
    return LocationID(BEST_LOCATION, id_, city_);
}

LocationID LocationID::toTopLevelLocation() const
{
    //WS_ASSERT(type_ == API_LOCATION || type_ == BEST_LOCATION);      // applicable only for API locations and best location
    return LocationID(type_, id_, QString());
}
