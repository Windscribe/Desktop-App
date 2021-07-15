#ifndef LOCATIONID_H
#define LOCATIONID_H

#include <QString>
#include <QMetaType>
#include <QHash>
#include "utils/protobuf_includes.h"

// Uniquely identifies a location among all locations (API locations, statis IPs locations, custom config locations, best location).
class LocationID
{
public:

    LocationID() : type_(INVALID_LOCATION), id_(0) {}

    static LocationID createTopApiLocationId(int id);
    static LocationID createTopStaticLocationId();
    static LocationID createTopCustomConfigsLocationId();
    static LocationID createApiLocationId(int id, const QString &city, const QString &nick);
    static LocationID createBestLocationId(int id);
    static LocationID createStaticIpsLocationId(const QString &city, const QString &ip);
    static LocationID createCustomConfigLocationId(const QString &filename);
    static LocationID createFromProtoBuf(const ProtoTypes::LocationId &lid);

    LocationID& operator=(LocationID rhs)
    {
        type_ = rhs.type_;
        id_ = rhs.id_;
        city_ = rhs.city_;
        return *this;
    }

    bool operator== (const LocationID &other) const
    {
        return (type_ == other.type_ && id_ == other.id_ && city_ == other.city_);
    }

    bool operator!= (const LocationID &other) const
    {
        return !operator==(other);
    }

    inline bool isValid() const { return type_ != INVALID_LOCATION; }
    inline bool isBestLocation() const { Q_ASSERT(type_ != INVALID_LOCATION); return type_ == BEST_LOCATION; }
    inline bool isCustomConfigsLocation() const { /*Q_ASSERT(type_ != INVALID_LOCATION);*/ return type_ == CUSTOM_CONFIGS_LOCATION; }
    inline bool isStaticIpsLocation() const { Q_ASSERT(type_ != INVALID_LOCATION); return type_ == STATIC_IPS_LOCATION; }
    bool isTopLevelLocation() const;

    LocationID bestLocationToApiLocation() const;
    LocationID apiLocationToBestLocation() const;
    LocationID toTopLevelLocation() const;
    ProtoTypes::LocationId toProtobuf() const;

    QString getHashString() const;

    int type() { return type_; }
    int id() { return id_; }
    QString city() { return city_; }

private:
    static constexpr int INVALID_LOCATION = 0;
    static constexpr int API_LOCATION = 1;
    static constexpr int BEST_LOCATION = 2;
    static constexpr int CUSTOM_CONFIGS_LOCATION = 3;
    static constexpr int STATIC_IPS_LOCATION = 4;

    LocationID(unsigned char type, int id, const QString &city) : type_(type), id_(id), city_(city) {}

    // the location is uniquely determined by these three values
    int type_;
    int id_;        // used for API_LOCATION and BEST_LOCATION
    QString city_;  // user for all locations:
                    // for API_LOCATION and BEST_LOCATION this is a city + nickname string
                    // for CUSTOM_OVPN_CONFIGS_LOCATION this is config filename
                    // for STATIC_IPS_LOCATION this is city + ip string
                    // for top level location - empty value
};

inline uint qHash(const LocationID &key, uint seed)
{
    return qHash(key.getHashString(), seed);
}

Q_DECLARE_METATYPE(LocationID)

#endif // LOCATIONID_H
