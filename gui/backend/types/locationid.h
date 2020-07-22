#ifndef LOCATIONID_H
#define LOCATIONID_H

#include <QString>
#include <QMetaType>
#include <QHash>
#include <QDataStream>

class LocationID
{
public:

    static const int BEST_LOCATION_ID = -1;
    static const int CUSTOM_OVPN_CONFIGS_LOCATION_ID = -2;
    static const int STATIC_IPS_LOCATION_ID = -3;
    static const int RIBBON_ITEM_STATIC_IP = -4;
    static const int RIBBON_ITEM_CONFIG = -5;

    LocationID() : locationId_(EMPTY_LOCATION) {}
    LocationID(int id, const QString &cityName);
    LocationID(int id);

    LocationID& operator=(LocationID rhs)
    {
        locationId_ = rhs.locationId_;
        city_ = rhs.city_;
        return *this;
    }

    bool operator== (const LocationID &other) const
    {
        return (locationId_ == other.locationId_ && city_ == other.city_);
    }

    inline bool isEmpty() const { return locationId_ == EMPTY_LOCATION; }
    inline int getId() const { return locationId_; }
    inline QString getCity() const { return city_; }
    inline void setId(int id) { locationId_ = id; }
    inline void setCity(const QString &city) { city_ = city; }
    QString getHashString() const;

private:
    const int EMPTY_LOCATION = -100;
    int locationId_;
    QString city_;
};

inline uint qHash(const LocationID &key, uint seed)
{
    return qHash(key.getHashString(), seed);
}

QDataStream &operator<<(QDataStream &out, const LocationID& rhs);
QDataStream &operator>>(QDataStream &in, LocationID &rhs);

Q_DECLARE_METATYPE(LocationID)


#endif // LOCATIONID_H
