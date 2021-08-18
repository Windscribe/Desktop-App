#include "pinghost_tcp.h"
#include <QTcpSocket>
#include <QTimer>
#include "../connectstatecontroller/iconnectstatecontroller.h"

PingHost_TCP::PingHost_TCP(QObject *parent, IConnectStateController *stateController) : QObject(parent), connectStateController_(stateController), bProxyEnabled_(true)
{
}

PingHost_TCP::~PingHost_TCP()
{
    clearPings();
}

void PingHost_TCP::addHostForPing(const QString &ip)
{
    if (!hostAlreadyPingingOrInWaitingQueue(ip))
    {
        waitingPingsQueue_.enqueue(ip);
        processNextPings();
    }
}

void PingHost_TCP::clearPings()
{
    for (QMap<QString, PingInfo *>::iterator it = pingingHosts_.begin(); it != pingingHosts_.end(); ++it)
    {
        delete it.value()->tcpSocket;
        delete it.value()->timer;
        delete it.value();
    }
    pingingHosts_.clear();
    waitingPingsQueue_.clear();
}

void PingHost_TCP::setProxySettings(const ProxySettings &proxySettings)
{
    proxySettings_ = proxySettings;
}

void PingHost_TCP::disableProxy()
{
    bProxyEnabled_ = false;
}

void PingHost_TCP::enableProxy()
{
    bProxyEnabled_ = true;
}

void PingHost_TCP::onSocketConnected()
{
    QTcpSocket *tcpSocket = (QTcpSocket *)sender();
    if (tcpSocket->write(".", 1) != 1)
    {
        processError(tcpSocket);
    }
}

void PingHost_TCP::onSocketBytesWritten(qint64 bytes)
{
    QTcpSocket *tcpSocket = (QTcpSocket *)sender();
    if (bytes == 1)
    {
        QString ip = tcpSocket->property("ip").toString();
        auto it = pingingHosts_.find(ip);
        if (it != pingingHosts_.end())
        {
            PingInfo *pingInfo = it.value();
            bool bFromDisconnectedState = pingInfo->tcpSocket->property("fromDisconnectedState").toBool();
            int timeMs = (int)pingInfo->elapsedTimer.elapsed();
            pingInfo->tcpSocket->deleteLater();
            pingInfo->timer->deleteLater();
            pingingHosts_.remove(ip);
            delete pingInfo;
            emit pingFinished(true, timeMs, ip, bFromDisconnectedState);
        }
        waitingPingsQueue_.removeAll(ip);
        processNextPings();
    }
    else
    {
        processError(tcpSocket);
    }
}

void PingHost_TCP::onSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    QObject *obj = sender();
    processError(obj);
}

void PingHost_TCP::onSocketTimeout()
{
    QObject *obj = sender();
    processError(obj);
}

inline bool PingHost_TCP::hostAlreadyPingingOrInWaitingQueue(const QString &ip)
{
    return pingingHosts_.find(ip) != pingingHosts_.end() || waitingPingsQueue_.indexOf(ip) != -1;
}

void PingHost_TCP::processNextPings()
{
    if (pingingHosts_.count() < MAX_PARALLEL_PINGS && !waitingPingsQueue_.isEmpty())
    {
        QString ip = waitingPingsQueue_.dequeue();

        PingInfo *pingInfo = new PingInfo();
        pingInfo->ip = ip;
        pingInfo->tcpSocket = new QTcpSocket(this);
        if (bProxyEnabled_ && proxySettings_.option() != PROXY_OPTION_NONE)
        {
            pingInfo->tcpSocket->setProxy(proxySettings_.getNetworkProxy());
        }
        pingInfo->timer = new QTimer(this);
        connect(pingInfo->timer, SIGNAL(timeout()), SLOT(onSocketTimeout()));
        connect(pingInfo->tcpSocket, SIGNAL(connected()), SLOT(onSocketConnected()));
        connect(pingInfo->tcpSocket, SIGNAL(bytesWritten(qint64)), SLOT(onSocketBytesWritten(qint64)));
        connect(pingInfo->tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(onSocketError(QAbstractSocket::SocketError)));
        if (connectStateController_)
        {
            pingInfo->tcpSocket->setProperty("fromDisconnectedState", connectStateController_->currentState() == CONNECT_STATE_DISCONNECTED || connectStateController_->currentState() == CONNECT_STATE_CONNECTING);
        }
        else
        {
            pingInfo->tcpSocket->setProperty("fromDisconnectedState", true);
        }

        pingInfo->tcpSocket->setProperty("ip", ip);
        pingInfo->timer->setProperty("ip", ip);
        pingingHosts_[ip] = pingInfo;

        pingInfo->elapsedTimer.start();
        pingInfo->timer->start(PING_TIMEOUT);
        pingInfo->tcpSocket->connectToHost(ip, 443, QTcpSocket::ReadWrite, QTcpSocket::IPv4Protocol);
    }
}

void PingHost_TCP::processError(QObject *obj)
{
    QString ip = obj->property("ip").toString();
    auto it = pingingHosts_.find(ip);
    if (it != pingingHosts_.end())
    {
        PingInfo *pingInfo = it.value();
        bool bFromDisconnectedState = pingInfo->tcpSocket->property("fromDisconnectedState").toBool();
        pingInfo->tcpSocket->deleteLater();
        pingInfo->timer->deleteLater();
        pingingHosts_.remove(ip);
        delete pingInfo;
        emit pingFinished(false, 0, ip, bFromDisconnectedState);
    }
    waitingPingsQueue_.removeAll(ip);
    processNextPings();
}

