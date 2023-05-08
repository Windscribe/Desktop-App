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

void PingHost_TCP::addHostForPing(const QString &id, const QString &ip)
{
    if (!hostAlreadyPingingOrInWaitingQueue(id))
    {
        QueueJob job;
        job.id = id;
        job.ip = ip;
        waitingPingsQueue_.enqueue(job);
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

void PingHost_TCP::setProxySettings(const types::ProxySettings &proxySettings)
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
        QString id = tcpSocket->property("id").toString();
        auto it = pingingHosts_.find(id);
        if (it != pingingHosts_.end())
        {
            PingInfo *pingInfo = it.value();
            bool bFromDisconnectedState = pingInfo->tcpSocket->property("fromDisconnectedState").toBool();
            int timeMs = (int)pingInfo->elapsedTimer.elapsed();
            pingInfo->tcpSocket->deleteLater();
            pingInfo->timer->deleteLater();
            pingingHosts_.remove(id);
            delete pingInfo;
            emit pingFinished(true, timeMs, id, bFromDisconnectedState);
        }
        removeFromQueue(id);
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

bool PingHost_TCP::hostAlreadyPingingOrInWaitingQueue(const QString &id)
{
    if (pingingHosts_.find(id) != pingingHosts_.end())
        return true;

    for (const auto &it : waitingPingsQueue_)
        if (it.id == id)
            return true;

    return false;
}

void PingHost_TCP::processNextPings()
{
    if (pingingHosts_.count() < MAX_PARALLEL_PINGS && !waitingPingsQueue_.isEmpty())
    {
        QueueJob job = waitingPingsQueue_.dequeue();

        PingInfo *pingInfo = new PingInfo();
        pingInfo->ip = job.ip;
        pingInfo->tcpSocket = new QTcpSocket(this);
        if (isProxyEnabled())
        {
            pingInfo->tcpSocket->setProxy(proxySettings_.getNetworkProxy());
        }
        pingInfo->timer = new QTimer(this);
        connect(pingInfo->timer, &QTimer::timeout, this, &PingHost_TCP::onSocketTimeout);
        connect(pingInfo->tcpSocket, &QTcpSocket::connected, this, &PingHost_TCP::onSocketConnected);
        connect(pingInfo->tcpSocket, &QTcpSocket::bytesWritten, this, &PingHost_TCP::onSocketBytesWritten);
        connect(pingInfo->tcpSocket, &QTcpSocket::errorOccurred, this, &PingHost_TCP::onSocketError);

        if (connectStateController_) {
            pingInfo->tcpSocket->setProperty("fromDisconnectedState", connectStateController_->currentState() == CONNECT_STATE_DISCONNECTED);
        }
        else {
            pingInfo->tcpSocket->setProperty("fromDisconnectedState", true);
        }

        pingInfo->tcpSocket->setProperty("id", job.id);
        pingInfo->timer->setProperty("id", job.id);
        pingingHosts_[job.id] = pingInfo;

        pingInfo->elapsedTimer.start();
        pingInfo->timer->start(PING_TIMEOUT);
        pingInfo->tcpSocket->connectToHost(job.ip, 443, QTcpSocket::ReadWrite, QTcpSocket::IPv4Protocol);
    }
}

void PingHost_TCP::processError(QObject *obj)
{
    QString id = obj->property("id").toString();
    auto it = pingingHosts_.find(id);
    if (it != pingingHosts_.end())
    {
        PingInfo *pingInfo = it.value();
        bool bFromDisconnectedState = pingInfo->tcpSocket->property("fromDisconnectedState").toBool();
        pingInfo->tcpSocket->deleteLater();
        pingInfo->timer->deleteLater();
        pingingHosts_.remove(id);
        delete pingInfo;
        emit pingFinished(false, 0, id, bFromDisconnectedState);
    }

    removeFromQueue(id);
    processNextPings();
}

void PingHost_TCP::removeFromQueue(const QString &id)
{
    QMutableListIterator<QueueJob> i(waitingPingsQueue_);
    while (i.hasNext()) {
        if (i.next().id == id)
            i.remove();
    }
}

bool PingHost_TCP::isProxyEnabled() const
{
    return (bProxyEnabled_ && proxySettings_.option() != PROXY_OPTION_NONE);
}
