#pragma once

#include <QString>
#include <QMetaType>
#include <QHash>
#include "utils/ws_assert.h"

// Uniquely identifies a location among all locations (API locations, statis IPs locations, custom config locations, best location).
class LocationID
{
public:

    LocationID() : type_(INVALID_LOCATION), id_(0) {}
    LocationID(int type, int id, const QString &city) : type_(type), id_(id), city_(city) {}

    static LocationID createTopApiLocationId(int id);
    static LocationID createTopStaticLocationId();
    static LocationID createTopCustomConfigsLocationId();
    static LocationID createApiLocationId(int id, const QString &city, const QString &nick);
    static LocationID createBestLocationId(int id);
    static LocationID createStaticIpsLocationId(const QString &city, const QString &ip);
    static LocationID createCustomConfigLocationId(const QString &filename);

    LocationID& operator=(LocationID rhs)
    {
        type_ = rhs.type_;
        id_ = rhs.id_;
        city_ = rhs.city_;
        return *this;
    }

    LocationID(const LocationID&) = default;

    bool operator== (const LocationID &other) const
    {
        return (type_ == other.type_ && id_ == other.id_ && city_ == other.city_);
    }

    bool operator!= (const LocationID &other) const
    {
        return !operator==(other);
    }

    inline bool isValid() const { return type_ != INVALID_LOCATION; }
    inline bool isBestLocation() const { return type_ == BEST_LOCATION; }
    inline bool isCustomConfigsLocation() const { return type_ == CUSTOM_CONFIGS_LOCATION; }
    inline bool isStaticIpsLocation() const { WS_ASSERT(type_ != INVALID_LOCATION); return type_ == STATIC_IPS_LOCATION; }
    bool isTopLevelLocation() const;

    LocationID bestLocationToApiLocation() const;
    LocationID apiLocationToBestLocation() const;
    LocationID toTopLevelLocation() const;

    QString getHashString() const;

    int type() const { return type_; }
    int id() const { return id_; }
    QString city() const { return city_; }

    friend QDataStream& operator <<(QDataStream &stream, const LocationID &l)
    {
        stream << versionForSerialization_;
        stream << l.type_ << l.id_ << l.city_;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream &stream, LocationID &l)
    {
        quint32 version;
        stream >> version;
        if (version > l.versionForSerialization_)
        {
            stream.setStatus(QDataStream::ReadCorruptData);
            return stream;
        }
        stream >> l.type_ >> l.id_ >> l.city_;
        return stream;
    }

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

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

inline size_t qHash(const LocationID &key, uint seed = 0)
{
    return qHash(key.getHashString(), seed);
}

Q_DECLARE_METATYPE(LocationID)
