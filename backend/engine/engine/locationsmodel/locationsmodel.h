#ifndef LOCATIONSMODEL_H
#define LOCATIONSMODEL_H

#include <QObject>
#include "engine/apiinfo/location.h"
#include "engine/apiinfo/staticips.h"
#include "engine/types/locationid.h"
#include "engine/proxy/proxysettings.h"
#include "locationitem.h"
#include "pingipscontroller.h"
#include "pingstorage.h"
#include "bestlocation.h"
#include "mutablelocationinfo.h"

namespace locationsmodel {

// Managing locations. Converts the apiinfo::Location vector to LocationItem vector.
// Add best location, static IPs, custom OVPN-configs location. Updates the ping speed for each location.
// thread safe access to public methods
class LocationsModel : public QObject
{
    Q_OBJECT
public:
    explicit LocationsModel(QObject *parent, IConnectStateController *stateController, INetworkStateManager *networkStateManager);

    void setLocations(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps);
    void clear();

    void setProxySettings(const ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

    QSharedPointer<MutableLocationInfo> getMutableLocationInfoById(const LocationID &locationId);

signals:
    void locationsUpdated(QSharedPointer<QVector<locationsmodel::LocationItem> > items);
    void locationPingTimeChanged(LocationID id, locationsmodel::PingTime timeMs);

    //void locationInfoChanged(const LocationID &LocationId, const QVector<ServerNode> &nodes, const QString &dnsHostName);
    //void customOvpnConfgsIpsChanged(const QStringList &ips);

private slots:
    void onPingInfoChanged(const QString &ip, int timems, bool isFromDisconnectedState);
    void onNeedIncrementPingIteration();

private:
    PingIpsController pingIpsController_;
    PingStorage pingStorage_;
    QVector<apiinfo::Location> locations_;
    apiinfo::StaticIps staticIps_;

    BestLocation bestLocation_;

    void detectBestLocation(bool isAllNodesInDisconnectedState);
    void generateLocationsUpdated();
    int calcLatency(const apiinfo::Location &l);

};

} //namespace locationsmodel

#endif // LOCATIONSMODEL_H
