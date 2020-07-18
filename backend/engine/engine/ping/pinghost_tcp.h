#ifndef PINGHOST_TCP_H
#define PINGHOST_TCP_H

#include <QVector>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QElapsedTimer>
#include <QQueue>
#include "Engine/Proxy/proxysettings.h"

class IConnectStateController;

class PingHost_TCP : public QObject
{
    Q_OBJECT
public:
    // stateController can be NULL, in this case not used
    explicit PingHost_TCP(QObject *parent, IConnectStateController *stateController);
    virtual ~PingHost_TCP();

    void addHostForPing(const QString &ip);
    void clearPings();

    void setProxySettings(const ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

signals:
    void pingFinished(bool bSuccess, int timems, const QString &ip, bool isFromDisconnectedState);

private slots:
    void onSocketConnected();
    void onSocketBytesWritten(qint64 bytes);
    void onSocketError(QAbstractSocket::SocketError socketError);
    void onSocketTimeout();

private:
    struct PingInfo
    {
        QString ip;
        QTcpSocket *tcpSocket;
        QTimer *timer;
        QElapsedTimer elapsedTimer;
    };

    enum {PING_TIMEOUT = 2000};
    const int MAX_PARALLEL_PINGS = 10;

    IConnectStateController *connectStateController_;
    ProxySettings proxySettings_;
    bool bProxyEnabled_;

    QMap<QString, PingInfo *> pingingHosts_;
    QQueue<QString> waitingPingsQueue_;

    bool hostAlreadyPingingOrInWaitingQueue(const QString &ip);
    void processNextPings();
    void processError(QObject *obj);
};

#endif // PINGHOST_TCP_H
