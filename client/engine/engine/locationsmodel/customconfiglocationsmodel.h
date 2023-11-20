#pragma once

#include <QHostInfo>
#include <QObject>

#include "baselocationinfo.h"
#include "engine/customconfigs/icustomconfig.h"
#include "engine/ping/pingmanager.h"
#include "types/location.h"
#include "types/locationid.h"

class INetworkDetectionManager;

namespace locationsmodel {

// Managing locations (generic API-locations and static IPs locations). Converts the apiinfo::Location vector to LocationItem vector.
// Detecting and adding best location. Makes and updates the ping for each location.
class CustomConfigLocationsModel : public QObject
{
    Q_OBJECT
public:
    explicit CustomConfigLocationsModel(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager, PingMultipleHosts *pingHosts);

    void setCustomConfigs(const QVector<QSharedPointer<const customconfigs::ICustomConfig>> &customConfigs);
    void clear();

    QSharedPointer<BaseLocationInfo> getMutableLocationInfoById(const LocationID &locationId);

signals:
    void locationsUpdated( QSharedPointer<types::Location> location);
    void locationPingTimeChanged(const LocationID &id, PingTime timeMs);

    void whitelistIpsChanged(const QStringList &ips);

    //void locationInfoChanged(const LocationID &LocationId, const QVector<ServerNode> &nodes, const QString &dnsHostName);
    //void customOvpnConfgsIpsChanged(const QStringList &ips);

private slots:
    void onPingInfoChanged(const QString &ip, int timems);
    void onDnsRequestFinished();

private:
    PingManager pingManager_;

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
