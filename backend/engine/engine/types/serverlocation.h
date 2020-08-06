#ifndef SERVERLOCATION_H
#define SERVERLOCATION_H

#include <QMap>
#include <QVector>
#include <QSharedPointer>
#include "servernode.h"

class ServerLocation
{
    friend StaticIpsLocation;

public:

    enum SERVER_LOCATION_TYPE { SERVER_LOCATION_DEFAULT, SERVER_LOCATION_CUSTOM_CONFIG, SERVER_LOCATION_BEST, SERVER_LOCATION_STATIC };

    struct CityNodes
    {
        QVector<int> nodesInds;  // this is indexes in ServerLocation(nodes vector)
    };

public:
    ServerLocation();

    bool initFromJson(QJsonObject &obj, QStringList &forceDisconnectNodes, QJsonObject &jsonProDataCenters);
    void transformToBestLocation(int selectedNodeIndForBestLocation, int bestLocationPingTimeMs, int bestLocationId);
    void transformToCustomOvpnLocation(QVector<ServerNode> &nodes);

    void writeToStream(QDataStream &stream);
    void readFromStream(QDataStream &stream, int revision);

    int getId() const;
    int nodesCount() const;
    QStringList getAllIps() const;
    QString getCountryCode() const;
    QString getName() const;
    int getP2P() const;
    QString getDnsHostname() const;
    bool isPremiumOnly() const;
    bool isForceExpand() const;
    bool isDisabled() const;

    bool isExistsHostname(const QString &hostname) const;
    int nodeIndByHostname(const QString &hostname) const;

    QStringList getCities();
    const QStringList &getProDataCenters() const;

    QVector<ServerNode> &getNodes();
    QVector<ServerNode> getCityNodes(const QString &cityname);
    void appendNode(const ServerNode &node);

    QString getStaticIpsDeviceName() const;

    int getBestLocationId() const;
    int getBestLocationSelectedNodeInd() const;
    int getBestLocationPingTimeMs() const;

    SERVER_LOCATION_TYPE getType() const { Q_ASSERT(isValid_); return type_; }

    bool isEqual(ServerLocation *other);

private:
    // don't forget modify function isEqual, if something changed in variables
    // data from API
    int id_;
    QString name_;
    QString countryCode_;
    int status_;
    int premiumOnly_;
    int p2p_;
    bool forceExpand_;
    QString dnsHostName_;
    QVector<ServerNode> nodes_;
    QStringList proDataCenters_;

    // internal state
    bool isValid_;

    QString staticIpsDeviceName_;

    int bestLocationId_;    // link id (which location is best location from locations list)
    int selectedNodeIndForBestLocation_;
    int bestLocationPingTimeMs_;

    QMap<QString, QSharedPointer<CityNodes> > citiesMap_;

    SERVER_LOCATION_TYPE type_;

    void makeInternalStates();
};

QVector< QSharedPointer<ServerLocation> > makeCopyOfServerLocationsVector(QVector< QSharedPointer<ServerLocation> > &serverLocations);

#endif // SERVERLOCATION_H
