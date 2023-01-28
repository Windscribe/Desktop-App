#ifndef PINGMANAGER_H
#define PINGMANAGER_H

#include <QVector>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QElapsedTimer>
#include <QRecursiveMutex>
#include "engine/types/apiinfo.h"
#include "engine/types/locationid.h"
#include "nodesspeedratings.h"
#include "nodesspeedstore.h"
#include "pingnodescontroller.h"
#include "pinglog.h"
#include "engine/networkstatemanager/inetworkstatemanager.h"
#include "pingtime.h"

class PingManager : public QObject
{
    Q_OBJECT
public:
    explicit PingManager(QObject *parent, NodesSpeedRatings *nodesSpeedRating, NodesSpeedStore *nodesSpeedStore, IConnectStateController *stateController, INetworkStateManager *networkStateManager);
    virtual ~PingManager();

    bool isInitialized();

    void updateServers(QVector< QSharedPointer<ServerLocation> > &newServers, bool bSkipCustomOvpnLocation);
    void clear();

    PingTime getPingTimeMs(const LocationID &locationId);
    int getSelectedNode(const LocationID &locationId);

signals:
    void connectionSpeedChanged(LocationID locationId, PingTime timeMs);
    void pingFinishedForAllNodes();

public slots:
    void init();
    void setProxySettings(const ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

private slots:
    void onPingStartedInDisconnectedState(const QString &ip);
    void onPingInfoChanged(const QString &ip, int timems, quint32 iteration);
    void onNodeSpeedRatingChanged(const QString &hostname);

private:

    struct PingInfo
    {
        bool isExists;
        PingTime timeMs;
        quint32 iteration;
    };

    struct LocationSpeedInfo
    {
        PingTime timeMs;
        int selectedNode;
        QVector<ServerNode> nodes;
        QVector<PingInfo> pingInfo;     //ping info for each node

        bool existThisLocation;  // used in function updateNodes for remove unused locations

        bool isAllNodesHavePingWithSameIteration() const;
        bool findPingInfoForNode(const ServerNode &sn, PingInfo &outPi) const;
        int findNodeInd(const ServerNode &sn) const;
    };

    bool isInitialized_;
    NodesSpeedRatings *nodesSpeedRating_;
    NodesSpeedStore *nodesSpeedStore_;
    QRecursiveMutex mutex_;

    QHash<LocationID, QSharedPointer<LocationSpeedInfo> > hashLocationsSpeedInfo_;

    bool isBestLocationDetermined_;
    int bestLocationId_;
    int selectedNodeIndForBestLocation_;


    QHash<LocationID, QString> hashNames_;  // need only for log (show userfriendly names for locations)

    PingNodesController *pingNodesController_;
    IConnectStateController *stateController_;
    INetworkStateManager *networkStateManager_;
    PingLog pingLog_;

    void addLocationOrUpdate(const LocationID &locationId, QVector<ServerNode> &nodes);
    void selectSpeedAndSelectedNodeForNewLocation(const LocationID &locationId, LocationSpeedInfo *lsi);
    bool recalculateSpeed(const LocationID &locationId, LocationSpeedInfo *lsi);
    bool isNodesVectorsEqual(const QVector<ServerNode> &nodes1, const QVector<ServerNode> &nodes2);
    QSharedPointer<LocationSpeedInfo> copyLocationSpeedInfoFromExisting(LocationSpeedInfo *lsi, QVector<ServerNode> &newNodes);
    int getAverageLatencyFromCitiesLatencies(const QVector<int> &latencies);
    void handlePingInfoChanged(const QString &ip, int timems, quint32 iteration, bool bForCities);
    void emitConnectionSpeedChangedWrapper(LocationID locationId, PingTime timeMs);
};

#endif // PINGMANAGER_H
