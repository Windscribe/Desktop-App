#ifndef PINGIPSCONTROLLER_H
#define PINGIPSCONTROLLER_H

#include <QDateTime>
#include <QObject>
#include <QSet>
#include "pingstorage.h"
#include "engine/ping/pinghost.h"
#include "pinglog.h"
#include "failedpinglogcontroller.h"
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"

namespace locationsmodel {

struct PingIpInfo
{
    QString ip_;     // ip or hostname
    PingHost::PING_TYPE pingType_;

    PingIpInfo(const QString &ip, PingHost::PING_TYPE pingType) : ip_(ip), pingType_(pingType) {}
    PingIpInfo() : pingType_(PingHost::PING_TCP) {}
};


// logic of ping all nodes (taken into account connected/disconnected state, latest ping time, repeat failed pings)
// starts ping on updateIps(...) and repeat ping every 24 hours
class PingIpsController : public QObject
{
    Q_OBJECT
public:

    explicit PingIpsController(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager, PingHost *pingHost, const QString &log_filename);

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

    FailedPingLogController failedPingLogController_;

    IConnectStateController *connectStateController_;
    INetworkDetectionManager *networkDetectionManager_;
    PingLog pingLog_;

    QHash<QString, PingNodeInfo> ips_;
    PingHost *pingHost_;
    QTimer pingTimer_;

    QDateTime dtNextPingTime_;
    bool isNeedPingForNextDisconnectState_;
};

} //namespace locationsmodel


#endif // PINGIPSCONTROLLER_H
