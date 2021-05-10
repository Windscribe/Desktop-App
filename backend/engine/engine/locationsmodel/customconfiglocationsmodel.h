#ifndef CUSTOMCONFIGLOCATIONSMODEL_H
#define CUSTOMCONFIGLOCATIONSMODEL_H

#include <QHostInfo>
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
    void onDnsRequestFinished();

private:
    PingStorage pingStorage_;

    PingIpsController pingIpsController_;

    struct IpItem
    {
        QString ip;
        PingTime pingTime;
    };

    struct RemoteItem
    {
        IpItem ipOrHostname;       // hostname or ip depending isHostname value
        bool isHostname;

        // make sense only for hostname
        bool isResolved;
        QVector<IpItem> ips;

        RemoteItem() : isHostname(false), isResolved(false) {}
    };

    struct CustomConfigWithPingInfo
    {
        QSharedPointer<const customconfigs::ICustomConfig> customConfig;

        QVector<RemoteItem> remotes;

        PingTime getPing() const;
        bool setPingTime(const QString &ip, PingTime pingTime);
    };

    QVector<CustomConfigWithPingInfo> pingInfos_;


    bool isAllResolved() const;
    void startPingAndWhitelistIps();
    void generateLocationsUpdated();
};

} //namespace locationsmodel

#endif // CUSTOMCONFIGLOCATIONSMODEL_H
