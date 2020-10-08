#ifndef PINGIPSCONTROLLER_H
#define PINGIPSCONTROLLER_H

#include <QDateTime>
#include <QObject>
#include <QSet>
#include "pingstorage.h"
#include "engine/ping/pinghost.h"
#include "pinglog.h"
#include "engine/networkstatemanager/inetworkstatemanager.h"


namespace locationsmodel {

struct PingIpInfo
{
    QString ip;     // ip or hostname
    PingHost::PING_TYPE pingType;
};


// logic of ping all nodes (taken into account connected/disconnected state, latest ping time, repeat failed pings)
// starts ping on updateIps(...) and repeat ping every 24 hours
class PingIpsController : public QObject
{
    Q_OBJECT
public:

    explicit PingIpsController(QObject *parent, IConnectStateController *stateController, INetworkStateManager *networkStateManager);

    void setProxySettings(const ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

    void updateIps(const QVector<PingIpInfo> &ips);

signals:
    void pingInfoChanged(const QString &ip, int timems, bool isFromDisconnectedState);
    void needIncrementPingIteration();


private slots:
    void onPingTimer();
    void onPingFinished(bool bSuccess, int timems, const QString &ip, bool isFromDisconnectedState);

private:
    static constexpr int PING_TIMER_INTERVAL = 1000;
    static constexpr int MAX_FAILED_PING_IN_ROW = 3;

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
    INetworkStateManager *networkStateManager_;
    PingLog pingLog_;

    QHash<QString, PingNodeInfo> ips_;
    PingHost pingHost_;
    QTimer pingTimer_;

    QDateTime dtNextPingTime_;
    bool isNeedPingForNextDisconnectState_;
};

} //namespace locationsmodel


#endif // PINGIPSCONTROLLER_H
