#ifndef LOCATIONSMODEL_H
#define LOCATIONSMODEL_H

#include <QObject>
#include "engine/apiinfo/location.h"
#include "engine/apiinfo/staticips.h"
#include "types/locationid.h"
#include "engine/proxy/proxysettings.h"
#include "locationitem.h"
#include "pingipscontroller.h"
#include "pingstorage.h"
#include "bestlocation.h"
#include "mutablelocationinfo.h"
#include "engine/customconfigs/icustomconfig.h"

namespace locationsmodel {

// Managing locations. Converts the apiinfo::Location vector to LocationItem vector.
// Add best location, static IPs, custom OVPN-configs location. Updates the ping speed for each location.
// thread safe access to public methods
class LocationsModel : public QObject
{
    Q_OBJECT
public:
    explicit LocationsModel(QObject *parent, IConnectStateController *stateController, INetworkStateManager *networkStateManager);

    void setLocations(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps, const QVector<QSharedPointer<const customconfigs::ICustomConfig>> &customConfigs);
    void setCustomConfigs();
    void clear();

    void setProxySettings(const ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

    QSharedPointer<MutableLocationInfo> getMutableLocationInfoById(const LocationID &locationId);

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
    PingIpsController pingIpsController_;
    PingStorage pingStorage_;
    QVector<apiinfo::Location> locations_;
    apiinfo::StaticIps staticIps_;
    QVector<QSharedPointer<const customconfigs::ICustomConfig>> customConfigs_;

    BestLocation bestLocation_;

    void detectBestLocation(bool isAllNodesInDisconnectedState);
    void generateLocationsUpdated();
    void whitelistIps();
    int calcLatency(const apiinfo::Location &l);

};

} //namespace locationsmodel

#endif // LOCATIONSMODEL_H
