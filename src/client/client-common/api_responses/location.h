#pragma once

#include <QVector>
#include <QJsonObject>
#include <QSharedPointer>
#include "group.h"

namespace wsnet { struct ServerLocation; }

namespace api_responses {

class LocationData : public QSharedData
{
public:
    LocationData() : id_(0), premiumOnly_(0),
        isValid_(false) {}

    LocationData(const LocationData &other)
        : QSharedData(other),
          id_(other.id_),
          name_(other.name_),
          countryCode_(other.countryCode_),
          shortName_(other.shortName_),
          premiumOnly_(other.premiumOnly_),
          dnsHostName_(other.dnsHostName_),
          groups_(other.groups_),
          isValid_(other.isValid_) {}
    ~LocationData() {}

    int id_;
    QString name_;
    QString countryCode_;
    QString shortName_;
    bool premiumOnly_;
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

    void initFromWsnet(const wsnet::ServerLocation &src);

    int getId() const { WS_ASSERT(d->isValid_); return d->id_; }
    QString getName() const { WS_ASSERT(d->isValid_); return d->name_; }
    QString getCountryCode() const { WS_ASSERT(d->isValid_); return d->countryCode_; }
    QString getShortName() const { WS_ASSERT(d->isValid_); return d->shortName_; }
    QString getDnsHostName() const { WS_ASSERT(d->isValid_); return d->dnsHostName_; }
    bool isPremiumOnly() const { WS_ASSERT(d->isValid_); return d->premiumOnly_; }

    int groupsCount() const { WS_ASSERT(d->isValid_); return d->groups_.count(); }
    Group getGroup(int ind) const { WS_ASSERT(d->isValid_); return d->groups_.at(ind); }
    void addGroup(const Group &group) { WS_ASSERT(d->isValid_); d->groups_.append(group); }

    QStringList getAllPingIps() const;

    bool operator == (const Location &other) const;
    bool operator != (const Location &other) const;

private:
    QSharedDataPointer<LocationData> d;
};


} //namespace api_responses

