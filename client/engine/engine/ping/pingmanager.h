#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QHash>

#include <wsnet/WSNet.h>

#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "pingstorage.h"

struct PingIpInfo
{
    QString ip;
    QString hostname;  // used for PING_CURL type
    QString city;      // only for log
    QString nick;      // only for log
    wsnet::PingType pingType;
};

// logic of ping all nodes (taken into account connected/disconnected state, latest ping time, repeat failed pings)
// starts ping on updateIps(...) and repeat ping every 24 hours
class PingManager : public QObject
{
    Q_OBJECT
public:
    explicit PingManager(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager,
                         const QString &storageSettingName);

    void updateIps(const QVector<PingIpInfo> &ips);
    void clearIps();

    bool isAllNodesHaveCurIteration() const;
    PingTime getPing(const QString &ip) const;

signals:
    void pingInfoChanged(const QString &ip, int timems);

private slots:
    void onPingTimer();

private:
    static constexpr int PING_TIMER_INTERVAL = 1000;
    static constexpr int MAX_FAILED_PING_IN_ROW = 3;
    static constexpr int MIN_DELAY_FOR_FAILED_IN_ROW_PINGS = 1;
    static constexpr int NEXT_PERIOD_SECS = 2*60*60*24;   //  How many secs to wait until the next ping (48 hours)

    IConnectStateController* const connectStateController_;
    INetworkDetectionManager* const networkDetectionManager_;

    QString storageSettingName_;
    PingStorage pingStorage_;

    struct PingIpState
    {
        PingIpInfo ipInfo;
        qint64 iterationTime;
        bool latestPingFailed;
        bool nowPinging;
        int failedPingsInRow;
        qint64 nextTimeForFailedPing;
        int curDelayForFailedPing = MIN_DELAY_FOR_FAILED_IN_ROW_PINGS;
        bool existThisIp;

        PingIpState()
        {
            resetState();
        }

        PingIpState(const PingIpInfo &ipInfo, qint64 iterTime, bool isLatestPingFailed) : ipInfo(ipInfo)
        {
            resetState();
            existThisIp = true;
            iterationTime = iterTime;
            latestPingFailed = isLatestPingFailed;
        }

        void resetState()
        {
            iterationTime = 0;
            latestPingFailed = false;
            nowPinging = false;
            failedPingsInRow = 0;
            nextTimeForFailedPing = 0;
            curDelayForFailedPing = MIN_DELAY_FOR_FAILED_IN_ROW_PINGS;
            existThisIp = false;
        }
    };

    QHash<QString, PingIpState> ips_;
    QTimer pingTimer_;
    bool isLogPings_;
    bool isUseIcmpPings_;

    void onPingFinished(const std::string &ip, bool isSuccess, std::int32_t timeMs, bool isFromDisconnectedVpnState);

    // Exponential Backoff algorithm, get next delay
    // We start re-ping failed nodes after 1 second. Then the delay increases according to the algorithm to a maximum of 1 minute.
    int exponentialBackoff_GetNextDelay(int curDelay, float factor = 2.0f, float jitter = 0.1f, float maxDelay = 60.0f);

    void addLog(const QString &tag, const QString &str);
};

