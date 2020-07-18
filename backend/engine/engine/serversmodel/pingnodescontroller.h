#ifndef PINGNODESCONTROLLER_H
#define PINGNODESCONTROLLER_H

#include <QDateTime>
#include <QObject>
#include <QSet>
#include "nodesspeedstore.h"
#include "engine/ping/pinghost.h"
#include "pinglog.h"
#include "engine/networkstatemanager/inetworkstatemanager.h"

class IConnectStateController;

// logic of ping all nodes (taken into account connected/disconnected state, latest ping time, repeat failed pings)
class PingNodesController : public QObject
{
    Q_OBJECT
public:

    struct PingNodeAndType
    {
        QString ip;
        PingHost::PING_TYPE pingType;
    };

    explicit PingNodesController(QObject *parent, IConnectStateController *stateController, NodesSpeedStore *nodesSpeedStore, INetworkStateManager *networkStateManager, PingLog &pingLog);

    void setProxySettings(const ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

signals:
    void pingStartedInDisconnectedState(const QString &ip);
    void pingInfoChanged(const QString &ip, int timems, quint32 iteration);

public slots:
    void updateNodes(QVector<PingNodesController::PingNodeAndType> nodes);

private slots:
    void onPingTimer();
    void onPingFinished(bool bSuccess, int timems, const QString &ip, bool isFromDisconnectedState);

private:
    const int PING_TIMER_INTERVAL = 1000;
    const int MAX_FAILED_PING_IN_ROW = 3;

    struct PingNodeInfo
    {
        bool isExistPingAttempt;
        bool latestPingFailed_;
        int failedPingsInRow;
        bool latestPingFromDisconnectedState_;
        qint64 nextTimeForFailedPing_;
        bool bNowPinging_;
        bool existThisIp;  // used in function updateNodes for remove unused ips
        PingHost::PING_TYPE pingType;
    };

    IConnectStateController *connectStateController_;
    NodesSpeedStore *nodesSpeedStore_;
    INetworkStateManager *networkStateManager_;
    PingLog &pingLog_;

    QHash<QString, PingNodeInfo> nodes_;
    PingHost pingHost_;
    QTimer pingTimer_;

    QDateTime dtNextPingTime_;
    bool isNeedPingForNextDisconnectState_;

    quint32 iteration_;   // 1 on app start and incremented only at ping time (it is 1 time in 24 hours)
};

#endif // PINGNODESCONTROLLER_H
