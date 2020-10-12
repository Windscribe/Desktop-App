#ifndef APILOCATIONSMODEL_H
#define APILOCATIONSMODEL_H

#include <QObject>
#include "engine/apiinfo/location.h"
#include "engine/apiinfo/staticips.h"
#include "types/locationid.h"
#include "engine/proxy/proxysettings.h"
#include "locationitem.h"
#include "pingipscontroller.h"
#include "pingstorage.h"
#include "bestlocation.h"
#include "baselocationinfo.h"
#include "engine/customconfigs/icustomconfig.h"
#include "hostnamesresolver.h"

namespace locationsmodel {

// Managing locations (generic API-locations and static IPs locations). Converts the apiinfo::Location vector to LocationItem vector.
// Detecting and adding best location. Makes and updates the ping for each location.
class ApiLocationsModel : public QObject
{
    Q_OBJECT
public:
    explicit ApiLocationsModel(QObject *parent, IConnectStateController *stateController, INetworkStateManager *networkStateManager, PingHost *pingHost);

    void setLocations(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps);
    void clear();

    QSharedPointer<BaseLocationInfo> getMutableLocationInfoById(const LocationID &locationId);

signals:
    void locationsUpdated( const LocationID &bestLocation, QSharedPointer<QVector<locationsmodel::LocationItem> > locations);
    void locationPingTimeChanged(const LocationID &id, locationsmodel::PingTime timeMs);

    void whitelistIpsChanged(const QStringList &ips);

    //void locationInfoChanged(const LocationID &LocationId, const QVector<ServerNode> &nodes, const QString &dnsHostName);
    //void customOvpnConfgsIpsChanged(const QStringList &ips);

private slots:
    void onPingInfoChanged(const QString &ip, int timems, bool isFromDisconnectedState);
    void onNeedIncrementPingIteration();

private:
    PingStorage pingStorage_;
    QVector<apiinfo::Location> locations_;
    apiinfo::StaticIps staticIps_;

    BestLocation bestLocation_;

    PingIpsController pingIpsController_;

    void detectBestLocation(bool isAllNodesInDisconnectedState);
    void generateLocationsUpdated();
    void whitelistIps();
    int calcLatency(const apiinfo::Location &l);

    bool isChanged(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps);

};

} //namespace locationsmodel

#endif // APILOCATIONSMODEL_H
