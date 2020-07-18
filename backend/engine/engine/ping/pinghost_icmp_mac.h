#ifndef PINGHOST_ICMP_MAC_H
#define PINGHOST_ICMP_MAC_H

#include <QObject>
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/proxy/proxysettings.h"
#include <QQueue>
#include <QMap>
#include <QProcess>

// todo proxy support for icmp ping
class PingHost_ICMP_mac : public QObject
{
    Q_OBJECT
public:
    explicit PingHost_ICMP_mac(QObject *parent, IConnectStateController *stateController);
    virtual ~PingHost_ICMP_mac();

    void addHostForPing(const QString &ip);
    void clearPings();

    void setProxySettings(const ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

signals:
    void pingFinished(bool bSuccess, int timems, const QString &ip, bool isFromDisconnectedState);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:

    struct PingInfo
    {
        QString ip;
        QProcess *process;
    };

    QMutex mutex_;
    IConnectStateController *connectStateController_;

    const int MAX_PARALLEL_PINGS = 10;
    QMap<QString, PingInfo *> pingingHosts_;
    QQueue<QString> waitingPingsQueue_;

    bool hostAlreadyPingingOrInWaitingQueue(const QString &ip);
    void processNextPings();
    int extractTimeMs(const QString &str);
};

#endif // PINGHOST_ICMP_MAC_H
