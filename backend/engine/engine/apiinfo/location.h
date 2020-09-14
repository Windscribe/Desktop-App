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
    explicit Location() { d = new LocationData; }
    Location(const Location &other) : d (other.d) { }

    bool initFromJson(QJsonObject &obj, QStringList &forceDisconnectNodes);
    void initFromProtoBuf(const ProtoApiInfo::Location &l);
    ProtoApiInfo::Location getProtoBuf() const;

    int getId() const { Q_ASSERT(d->isValid_); return d->id_; }
    QString getName() const { Q_ASSERT(d->isValid_); return d->name_; }
    QString getCountryCode() const { Q_ASSERT(d->isValid_); return d->countryCode_; }
    bool isPremiumOnly() const { Q_ASSERT(d->isValid_); return d->premiumOnly_; }
    int getP2P() const { Q_ASSERT(d->isValid_); return d->p2p_; }

    int groupsCount() const { Q_ASSERT(d->isValid_); return d->groups_.count(); }
    Group getGroup(int ind) const { Q_ASSERT(d->isValid_); return d->groups_.at(ind); }




    /*void transformToBestLocation(int selectedNodeIndForBestLocation, int bestLocationPingTimeMs, int bestLocationId);
    void transformToCustomOvpnLocation(QVector<ServerNode> &nodes);

    void writeToStream(QDataStream &stream);
    void readFromStream(QDataStream &stream, int revision);

    int getId() const;
    int nodesCount() const;*/
    QStringList getAllIps() const;
    /*QString getCountryCode() const;
    QString getName() const;
    int getP2P() const;
    QString getDnsHostname() const;
    bool isPremiumOnly() const;

    bool isExistsHostname(const QString &hostname) const;
    int nodeIndByHostname(const QString &hostname) const;

    QStringList getCities();
    const QStringList &getProDataCenters() const;
    void appendProDataCentre(const QString &name);

    QVector<ServerNode> &getNodes();
    QVector<ServerNode> getCityNodes(const QString &cityname);
    void appendNode(const ServerNode &node);

    QString getStaticIpsDeviceName() const;

    int getBestLocationId() const;
    int getBestLocationSelectedNodeInd() const;
    int getBestLocationPingTimeMs() const;

    SERVER_LOCATION_TYPE getType() const { Q_ASSERT(isValid_); return type_; }

    bool isEqual(ServerLocation *other);*/

private:
    QSharedDataPointer<LocationData> d;
};


} //namespace apiinfo

#endif // APIINFO_LOCATION_H
