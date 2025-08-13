#pragma once

#include <QObject>
#include <QHash>

#include "baselocationinfo.h"
#include "bestlocation.h"
#include "api_responses/location.h"
#include "api_responses/staticips.h"
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "engine/ping/pingmanager.h"
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
    explicit ApiLocationsModel(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager);

    void setLocations(const QVector<api_responses::Location> &locations, const api_responses::StaticIps &staticIps);
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
    void onPingInfoChanged(const QString &ip, int timems);

private:
    QVector<api_responses::Location> locations_;
    api_responses::StaticIps staticIps_;
    BestLocation bestLocation_;
    PingManager pingManager_;

private:
    void detectBestLocation(bool isAllNodesInDisconnectedState);
    BestAndAllLocations generateLocationsUpdated(const QVector<api_responses::Location> &prevLocations);
    void sendLocationsUpdated(const QVector<api_responses::Location> &prevLocations);
    void whitelistIps();

    bool isChanged(const QVector<api_responses::Location> &locations, const api_responses::StaticIps &staticIps);
    PingTime getPrevPing(const QVector<api_responses::Location> &prevLocations, int countryId, int cityId);
};

} //namespace locationsmodel
