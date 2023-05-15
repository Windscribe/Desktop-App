#pragma once

#include <QMap>
#include <QObject>
#include <QProcess>
#include <QQueue>
#include <QRecursiveMutex>

#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "types/proxysettings.h"

// todo proxy support for icmp ping
class PingHost_ICMP_mac : public QObject
{
    Q_OBJECT
public:
    explicit PingHost_ICMP_mac(QObject *parent, IConnectStateController *stateController);
    virtual ~PingHost_ICMP_mac();

    void addHostForPing(const QString &id, const QString &ip);
    void clearPings();

    void setProxySettings(const types::ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

signals:
    void pingFinished(bool bSuccess, int timems, const QString &id, bool isFromDisconnectedState);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:

    struct QueueJob
    {
        QString id;
        QString ip;
    };

    struct PingInfo
    {
        QString ip;
        QProcess *process;
    };

    IConnectStateController* const connectStateController_;

    static constexpr int MAX_PARALLEL_PINGS = 10;
    QRecursiveMutex mutex_;
    QMap<QString, PingInfo *> pingingHosts_;
    QQueue<QueueJob> waitingPingsQueue_;

    bool hostAlreadyPingingOrInWaitingQueue(const QString &id);
    void processNextPings();
    int extractTimeMs(const QString &str);
    void removeFromQueue(const QString &id);
};
