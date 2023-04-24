#pragma once

#include <QObject>
#include <QHash>

#include "baselocationinfo.h"
#include "bestlocation.h"
#include "engine/apiinfo/location.h"
#include "engine/apiinfo/staticips.h"
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "pingipscontroller.h"
#include "pingstorage.h"
#include "types/location.h"
#include "types/locationid.h"

namespace locationsmodel {

struct BestAndAllLocations {
    LocationID bestLocation;
    QString staticIpDeviceName;
    QSharedPointer<QVector<types::Location> > locations;
};

// Managing locations (generic API-locations and static IPs locations). Converts the apiinfo::Location vector to LocationItem vector.
// Detecting and adding best location. Makes and updates the ping for each location.
class ApiLocationsModel : public QObject
{
    Q_OBJECT
public:
    explicit ApiLocationsModel(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager, PingHost *pingHost);

    void setLocations(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps);
    void clear();

    QSharedPointer<BaseLocationInfo> getMutableLocationInfoById(const LocationID &locationId);

signals:
    void locationsUpdated( const LocationID &bestLocation, const QString &staticIpDeviceName, QSharedPointer<QVector<types::Location> > locations);
    void locationsUpdatedCliOnly(const LocationID &bestLocation, QSharedPointer<QVector<types::Location> > locations);
    void locationPingTimeChanged(const LocationID &id, PingTime timeMs);
    void bestLocationUpdated( const LocationID &bestLocation);
    void whitelistIpsChanged(const QStringList &ips);

    //void locationInfoChanged(const LocationID &LocationId, const QVector<ServerNode> &nodes, const QString &dnsHostName);
    //void customOvpnConfgsIpsChanged(const QStringList &ips);

private slots:
    void onPingInfoChanged(const QString &id, int timems);
    void onNeedIncrementPingIteration();

private:
    ApiPingStorage pingStorage_;
    QVector<apiinfo::Location> locations_;
    apiinfo::StaticIps staticIps_;

    BestLocation bestLocation_;

    PingIpsController pingIpsController_;

private:
    void detectBestLocation(bool isAllNodesInDisconnectedState);
    BestAndAllLocations generateLocationsUpdated();
    void sendLocationsUpdated();
    void whitelistIps();

    bool isChanged(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps);
};

} //namespace locationsmodel
