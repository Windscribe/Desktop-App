#ifndef SERVERSMODEL_H
#define SERVERSMODEL_H

#include <QTimer>
#include "iserversmodel.h"
#include "engine/networkstatemanager/inetworkstatemanager.h"
#include "nodesspeedratings.h"
#include "pingmanager.h"
#include "resolvehostnamesforcustomovpn.h"
#include "../apiinfo/location.h"

bool operator<(const ModelExchangeCityItem& a, const ModelExchangeCityItem& b);

// thread safe access to public methods
class ServersModel : public IServersModel
{
    Q_OBJECT
public:
    explicit ServersModel(QObject *parent, IConnectStateController *stateController, INetworkStateManager *networkStateManager,
                          NodesSpeedRatings *nodesSpeedRating, NodesSpeedStore *nodesSpeedStore);
    virtual ~ServersModel();

    void updateServers(const QVector<apiinfo::Location> &newServers, bool bSkipCustomOvpnDnsResolution = false);
    void clear();

    void setSessionStatus(bool bFree);

    PingTime getPingTimeMsForLocation(const LocationID &locationId) override;
    void getNameAndCountryByLocationId(LocationID &locationId, QString &outName, QString &outCountry) override;
    QSharedPointer<MutableLocationInfo> getMutableLocationInfoById(LocationID locationId) override;
    LocationID getLocationIdByName(const QString &location);

    QStringList getCitiesForLocationId(int locationId);

    void setProxySettings(const ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

signals:
    void locationInfoChanged(const LocationID &LocationId, const QVector<ServerNode> &nodes, const QString &dnsHostName);
    void customOvpnConfgsIpsChanged(const QStringList &ips);

private slots:
    void onCustomOvpnDnsResolved(QSharedPointer<ServerLocation> location);

private:
    QVector<QSharedPointer<ServerLocation> > servers_;
    QMap<int, QSharedPointer<ServerLocation> > serversMap_;
    enum { COLUMNS_COUNT = 1 };

    bool bFreeSessionStatusInitialized_;
    bool bFreeSessionStatus_;

    PingManager *pingManager_;
    QThread *pingManagerThread_;

    ResolveHostnamesForCustomOvpn resolveHostnamesForCustomOvpn_;
    enum CUSTOM_OVPN_STATE {CUSTOM_OVPN_NO, CUSTOM_OVPN_IN_PROGRESS, CUSTOM_OVPN_FINISHED};
    CUSTOM_OVPN_STATE customOvpnDnsState_;

    QMutex mutex_;

    void clearServers();
    ModelExchangeLocationItem serverLocationToModelExchangeLocationItem(ServerLocation &sl);
    ServerLocation *findServerLocationById(int id);
    PingTime getPingTimeMsForCustomOvpnConfig(const LocationID &locationId);
    void setCustomOvpnConfigIpsToFirewall(ServerLocation &sl);
};

#endif // SERVERSMODEL_H
