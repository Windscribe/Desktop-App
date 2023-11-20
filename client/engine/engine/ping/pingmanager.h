#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QHash>

#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "pingmultiplehosts.h"
#include "pingstorage.h"
#include "failedpinglogcontroller.h"
#include "pinglog.h"

struct PingIpInfo
{
    QString ip;
    QString hostname;  // used for PING_CURL type
    QString city;      // only for log
    QString nick;      // only for log
    PingType pingType;
};

// logic of ping all nodes (taken into account connected/disconnected state, latest ping time, repeat failed pings)
// starts ping on updateIps(...) and repeat ping every 24 hours
class PingManager : public QObject
{
    Q_OBJECT
public:
    explicit PingManager(QObject *parent, IConnectStateController *stateController, INetworkDetectionManager *networkDetectionManager, PingMultipleHosts *pingHosts,
                               const QString &storageSettingName, const QString &log_filename);

    void updateIps(const QVector<PingIpInfo> &ips);
    void clearIps();

    bool isAllNodesHaveCurIteration() const;
    PingTime getPing(const QString &ip) const;

signals:
    void pingInfoChanged(const QString &ip, int timems);

private slots:
    void onPingTimer();
    void onPingFinished(bool success, int timems, const QString &ip, bool isFromDisconnectedState);

private:
    static constexpr int PING_TIMER_INTERVAL = 1000;
    static constexpr int MAX_FAILED_PING_IN_ROW = 3;
    static constexpr int NEXT_PERIOD_SECS = 2*60*60*24;   //  How many secs to wait until the next ping (48 hours)

    PingMultipleHosts* const pingHosts_;
    IConnectStateController* const connectStateController_;
    INetworkDetectionManager* const networkDetectionManager_;

    PingStorage pingStorage_;
    FailedPingLogController failedPingLogController_;
    PingLog pingLog_;

    struct PingIpState
    {
        PingIpInfo ipInfo;
        qint64 iterationTime;
        bool latestPingFailed;
        int failedPingsInRow;
        qint64 nextTimeForFailedPing;
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
            failedPingsInRow = 0;
            nextTimeForFailedPing = 0;
            existThisIp = false;
        }
    };

    QHash<QString, PingIpState> ips_;
    QTimer pingTimer_;
};

