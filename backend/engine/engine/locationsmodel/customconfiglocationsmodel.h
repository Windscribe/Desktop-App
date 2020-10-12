#ifndef CUSTOMCONFIGLOCATIONSMODEL_H
#define CUSTOMCONFIGLOCATIONSMODEL_H

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
class CustomConfigLocationsModel : public QObject
{
    Q_OBJECT
public:
    explicit CustomConfigLocationsModel(QObject *parent, IConnectStateController *stateController, INetworkStateManager *networkStateManager, PingHost *pingHost);

    void setCustomConfigs(const QVector<QSharedPointer<const customconfigs::ICustomConfig>> &customConfigs);
    void clear();

    QSharedPointer<BaseLocationInfo> getMutableLocationInfoById(const LocationID &locationId);

signals:
    void locationsUpdated( QSharedPointer<QVector<locationsmodel::LocationItem> > locations);
    void locationPingTimeChanged(const LocationID &id, locationsmodel::PingTime timeMs);

    void whitelistIpsChanged(const QStringList &ips);

    //void locationInfoChanged(const LocationID &LocationId, const QVector<ServerNode> &nodes, const QString &dnsHostName);
    //void customOvpnConfgsIpsChanged(const QStringList &ips);

private slots:
    void onPingInfoChanged(const QString &ip, int timems, bool isFromDisconnectedState);
    void onNeedIncrementPingIteration();
    void onHostnamesResolved();

    void onTimer();

private:
    PingStorage pingStorage_;
    QVector<QSharedPointer<const customconfigs::ICustomConfig>> customConfigs_;

    HostnamesResolver hostnameResolver_;

    PingIpsController pingIpsController_;


    void generateLocationsUpdated();
    void whitelistIps();

    bool isChanged(const QVector<apiinfo::Location> &locations, const apiinfo::StaticIps &staticIps, const QVector<QSharedPointer<const customconfigs::ICustomConfig>> &customConfigs);

};

} //namespace locationsmodel

#endif // CUSTOMCONFIGLOCATIONSMODEL_H
