#pragma once

#include <QDateTime>
#include <QHash>
#include <QObject>

#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "engine/ping/pinghost.h"
#include "failedpinglogcontroller.h"
#include "pinglog.h"

namespace locationsmodel {

struct PingIpInfo
{
    QString ip_;
    QString hostname_;  // used for PING_CURL type
    QString city_;
    QString nick_;
    PingHost::PING_TYPE pingType_;

    PingIpInfo(const QString &ip, const QString &hostname, const QString &city, const QString &nick, PingHost::PING_TYPE pingType) :
        ip_(ip), hostname_(hostname), city_(city), nick_(nick), pingType_(pingType) {}
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
    void pingInfoChanged(const QString &ip, int timems);
    void needIncrementPingIteration();

private slots:
    void onPingTimer();
    void onPingFinished(bool success, int timems, const QString &ip, bool isFromDisconnectedState);

private:
    static constexpr int PING_TIMER_INTERVAL = 1000;
    static constexpr int MAX_FAILED_PING_IN_ROW = 3;

    PingHost* const pingHost_;
    IConnectStateController* const connectStateController_;
    INetworkDetectionManager* const networkDetectionManager_;

    FailedPingLogController failedPingLogController_;
    PingLog pingLog_;

    struct PingNodeInfo
    {
        PingIpInfo ipInfo_;
        bool isExistPingAttempt_ = false;
        bool latestPingFailed_ = false;
        bool nowPinging_ = false;
        int failedPingsInRow_ = 0;
        qint64 nextTimeForFailedPing_ = 0;
        bool existThisIp = false;

        PingNodeInfo() {}
        PingNodeInfo(const PingIpInfo &ipInfo) : ipInfo_(ipInfo), existThisIp(true) {}
    };

    QHash<QString, PingNodeInfo> ips_;

    QTimer pingTimer_;
    QDateTime dtNextPingTime_;
};

} //namespace locationsmodel
