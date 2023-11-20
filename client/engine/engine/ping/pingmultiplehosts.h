#pragma once

#include <QObject>
#include <QQueue>
#include "types/proxysettings.h"
#include "ipinghost.h"

class IConnectStateController;
class NetworkAccessManager;

enum class PingType { kTcp, kIcmp, kCurl };

class PingMultipleHosts : public QObject
{
    Q_OBJECT
public:
    // connectStateController can be NULL, in this case not used
    explicit PingMultipleHosts(QObject *parent, IConnectStateController *connectStateController, NetworkAccessManager *networkAccessManager);

    // hostname used only for Curl-ping
    void addHostForPing(const QString &ip, const QString &hostname, PingType pingType);

    void setProxySettings(const types::ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

signals:
    void pingFinished(bool success, int timems, const QString &ip, bool isFromDisconnectedState);

private slots:
    void onPingFinished(bool bSuccess, int timems, const QString &ip);

private:
    IConnectStateController *connectStateController_;
    NetworkAccessManager *networkAccessManager_;
    types::ProxySettings proxySettings_;
    bool isProxyEnabled_ = false;

    QSet<QString> pingingHosts_;
    QQueue<IPingHost *> waitingPingsQueue_;

    static constexpr int MAX_PARALLEL_PINGS = 10;
    int curParallelPings_ = 0;

    void processNextPingsInQueue();
    IPingHost *createPingHost(const QString &ip, const QString &hostname, PingType pingType);
};
