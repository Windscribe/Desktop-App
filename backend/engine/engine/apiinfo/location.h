#ifndef APIINFO_LOCATION_H
#define APIINFO_LOCATION_H

#include <QVector>
#include <QJsonObject>
#include <QSharedPointer>
#include "group.h"
#include "ipc/generated_proto/apiinfo.pb.h"

namespace apiinfo {

enum SERVER_LOCATION_TYPE { SERVER_LOCATION_DEFAULT, SERVER_LOCATION_CUSTOM_CONFIG, SERVER_LOCATION_BEST, SERVER_LOCATION_STATIC };

class LocationData : public QSharedData
{
public:
    LocationData() : id_(0), premiumOnly_(0), p2p_(0),
        isValid_(false),
        type_(SERVER_LOCATION_DEFAULT) {}

    LocationData(const LocationData &other)
        : QSharedData(other),
          id_(other.id_),
          name_(other.name_),
          countryCode_(other.countryCode_),
          premiumOnly_(other.premiumOnly_),
          p2p_(other.p2p_),
          dnsHostName_(other.dnsHostName_),
          groups_(other.groups_),
          isValid_(other.isValid_),
          type_(other.type_) {}
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
    SERVER_LOCATION_TYPE type_;
};

// implicitly shared class Location
class Location
{
public:
    explicit Location() : d(new LocationData) {}
    Location(const Location &other) : d (other.d) {}

    bool initFromJson(QJsonObject &obj, QStringList &forceDisconnectNodes);
    void initFromProtoBuf(const ProtoApiInfo::Location &l);
    ProtoApiInfo::Location getProtoBuf() const;

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

    bool operator== (const Location &other) const
    {
        return d->id_ == other.d->id_ &&
               d->name_ == other.d->name_ &&
               d->countryCode_ == other.d->countryCode_ &&
               d->premiumOnly_ == other.d->premiumOnly_ &&
               d->p2p_ == other.d->p2p_ &&
               d->dnsHostName_ == other.d->dnsHostName_ &&
               d->groups_ == other.d->groups_ &&
               d->isValid_ == other.d->isValid_ &&
               d->type_ == other.d->type_;
    }

    bool operator!= (const Location &other) const
    {
        return !operator==(other);
    }

private:
    QSharedDataPointer<LocationData> d;
};


} //namespace apiinfo

#endif // APIINFO_LOCATION_H
