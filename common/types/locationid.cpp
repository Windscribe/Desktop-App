#include "locationid.h"

const int typeIdLocationId = qRegisterMetaType<LocationID>("LocationID");

QString LocationID::getHashString() const
{
    Q_ASSERT(type_ != INVALID_LOCATION);
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

LocationID LocationID::createFromProtoBuf(const ProtoTypes::LocationId &lid)
{
    return LocationID(lid.type(), lid.id(), QString::fromStdString(lid.city()));
}

bool LocationID::isTopLevelLocation() const
{
    Q_ASSERT(type_ != INVALID_LOCATION);
    return city_.isEmpty();
}

LocationID LocationID::bestLocationToApiLocation() const
{
    Q_ASSERT(type_ == BEST_LOCATION);
    return LocationID(API_LOCATION, id_, city_);
}

LocationID LocationID::apiLocationToBestLocation() const
{
    Q_ASSERT(type_ == API_LOCATION);
    return LocationID(BEST_LOCATION, id_, city_);
}

LocationID LocationID::toTopLevelLocation() const
{
    Q_ASSERT(type_ == API_LOCATION || type_ == BEST_LOCATION);      // applicable only for API locations and best location
    return LocationID(type_, id_, QString());
}

ProtoTypes::LocationId LocationID::toProtobuf() const
{
    ProtoTypes::LocationId lid;
    lid.set_type(type_);
    lid.set_id(id_);
    lid.set_city(city_.toStdString());
    return lid;
}
