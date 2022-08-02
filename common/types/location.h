#ifndef TYPES_LOCATION_H
#define TYPES_LOCATION_H

#include <QVector>
#include <QJsonObject>
#include <QSharedPointer>
#include "group.h"

namespace types {

class LocationData : public QSharedData
{
public:
    LocationData() : id_(0), premiumOnly_(0), p2p_(0),
        isValid_(false) {}

    LocationData(const LocationData &other)
        : QSharedData(other),
          id_(other.id_),
          name_(other.name_),
          countryCode_(other.countryCode_),
          premiumOnly_(other.premiumOnly_),
          p2p_(other.p2p_),
          dnsHostName_(other.dnsHostName_),
          groups_(other.groups_),
          isValid_(other.isValid_) {}
    ~LocationData() {}

    int id_;
    QString name_;
    QString countryCode_;
    int premiumOnly_;
    int p2p_;
    QString dnsHostName_;

    QVector<Group> groups_;

    // internal state
    bool isValid_;
};

// implicitly shared class Location
class Location
{
public:
    explicit Location() : d(new LocationData) {}
    Location(const Location &other) : d (other.d) {}

    bool initFromJson(const QJsonObject &obj, QStringList &forceDisconnectNodes);

    int getId() const { Q_ASSERT(d->isValid_); return d->id_; }
    QString getName() const { Q_ASSERT(d->isValid_); return d->name_; }
    QString getCountryCode() const { Q_ASSERT(d->isValid_); return d->countryCode_; }
    QString getDnsHostName() const { Q_ASSERT(d->isValid_); return d->dnsHostName_; }
    bool isPremiumOnly() const { Q_ASSERT(d->isValid_); return d->premiumOnly_; }
    int getP2P() const { Q_ASSERT(d->isValid_); return d->p2p_; }

    int groupsCount() const { Q_ASSERT(d->isValid_); return d->groups_.count(); }
    Group getGroup(int ind) const { Q_ASSERT(d->isValid_); return d->groups_.at(ind); }
    void addGroup(const Group &group) { Q_ASSERT(d->isValid_); d->groups_.append(group); }

    QStringList getAllPingIps() const;

    bool operator == (const Location &other) const
    {
        return d->id_ == other.d->id_ &&
               d->name_ == other.d->name_ &&
               d->countryCode_ == other.d->countryCode_ &&
               d->premiumOnly_ == other.d->premiumOnly_ &&
               d->p2p_ == other.d->p2p_ &&
               d->dnsHostName_ == other.d->dnsHostName_ &&
               d->groups_ == other.d->groups_ &&
               d->isValid_ == other.d->isValid_;
    }

    bool operator != (const Location &other) const
    {
        return !operator==(other);
    }

    friend QDataStream& operator <<(QDataStream& stream, const Location& l)
    {
        Q_ASSERT(l.d->isValid_);
        stream << versionForSerialization_;

        stream << l.d->id_ << l.d->name_ << l.d->countryCode_ << l.d->premiumOnly_ << l.d->p2p_ << l.d->dnsHostName_ << l.d->groups_;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream& stream, Location& l)
    {
        quint32 version;
        stream >> version;
        Q_ASSERT(version == versionForSerialization_);
        if (version != versionForSerialization_)
        {
            l.d->isValid_ = false;
            return stream;
        }

        stream >> l.d->id_ >> l.d->name_ >> l.d->countryCode_ >> l.d->premiumOnly_ >> l.d->p2p_ >> l.d->dnsHostName_ >> l.d->groups_;
        l.d->isValid_ = true;

        return stream;
    }


private:
    QSharedDataPointer<LocationData> d;
    static constexpr quint32 versionForSerialization_ = 1;
};


} //namespace types

#endif // TYPES_LOCATION_H
