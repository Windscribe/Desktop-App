#pragma once

#include <QMap>
#include <QObject>
#include <QQueue>
#include <QRecursiveMutex>
#include <QString>

#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "types/proxysettings.h"

// todo proxy support for icmp ping

class PingRequest;

class PingHost_ICMP_win : public QObject
{
    Q_OBJECT
public:
    explicit PingHost_ICMP_win(QObject *parent, IConnectStateController *stateController);
    virtual ~PingHost_ICMP_win();

    void addHostForPing(const QString &ip);
    void clearPings();

    void setProxySettings(const types::ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

signals:
    void pingFinished(bool success, int timems, const QString &ip, bool isFromDisconnectedState);

private slots:
    void onPingRequestFinished(bool success, const QString &ip);

private:
    IConnectStateController* const connectStateController_;

    QRecursiveMutex mutex_;
    QMap<QString, PingRequest*> pingingHosts_;
    QQueue<QString> waitingPingsQueue_;

    void sendNextPing();
};
