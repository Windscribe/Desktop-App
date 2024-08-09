#include "socksproxyconnectionmanager.h"

#include <QThread>
#include <QTimer>
#include "utils/ws_assert.h"

namespace SocksProxyServer {

SocksProxyConnectionManager::SocksProxyConnectionManager(QObject *parent, int threadsCount, ConnectedUsersCounter *usersCounter) : QObject(parent),
    usersCounter_(usersCounter)
{
    WS_ASSERT(threadsCount > 0);
    for (int i = 0; i < threadsCount; i++)
    {
        QThread *thread = new QThread(this);
        threads_[thread] = 0;
        thread->start(QThread::LowPriority);
    }
}

void SocksProxyConnectionManager::newConnection(qintptr socketDescriptor)
{
#ifdef Q_OS_WIN
    SOCKADDR_IN  addr = {0};
    int addr_len = sizeof(addr);
#elif defined (Q_OS_MACOS) || defined (Q_OS_LINUX)
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
#endif
    getpeername(socketDescriptor, (sockaddr*)&addr, &addr_len);
    char *ip = inet_ntoa(addr.sin_addr);
    usersCounter_->newUserConnected(ip);

    QThread *thread = getLessBusyThread();
    SocksProxyConnection *connection = new SocksProxyConnection(socketDescriptor, ip);
    connect(connection, &SocksProxyConnection::finished, this, &SocksProxyConnectionManager::onConnectionFinished);
    addConnectionToThread(thread, connection);
    //qCDebug(LOG_SOCKS_SERVER) << "Count of connections:" << connections_.count();
}

void SocksProxyConnectionManager::closeAllConnections()
{
    for(auto c : connections_.keys())
    {
        QMetaObject::invokeMethod(c, "forceClose", Qt::QueuedConnection);
    }
}

void SocksProxyConnectionManager::stop()
{
    for(auto thread : threads_.keys())
    {
        thread->exit();
    }
    for(auto thread : threads_.keys())
    {
        thread->wait();
    }
}

void SocksProxyConnectionManager::onConnectionFinished(const QString &hostname)
{
    usersCounter_->userDiconnected(hostname);

    SocksProxyConnection *connection = static_cast<SocksProxyConnection *>(sender());
    //qCDebug(LOG_SOCKS_SERVER) << "Connection finished:" << connection;
    QMap<SocksProxyConnection *, QThread *>::iterator it = connections_.find(connection);
    WS_ASSERT(it != connections_.end());

    QMap<QThread *, quint32>::iterator itThread = threads_.find(it.value());
    WS_ASSERT(itThread != threads_.end());
    itThread.value()--;

    it.key()->deleteLater();
    connections_.erase(it);

    //qCDebug(LOG_SOCKS_SERVER) << "Count of connections:" << connections_.count();
}

QThread *SocksProxyConnectionManager::getLessBusyThread()
{
    WS_ASSERT(threads_.count() > 0);
    quint32 min = threads_.begin().value();
    QThread *thread = threads_.begin().key();
    for (QMap<QThread *, quint32>::iterator it = threads_.begin(); it != threads_.end(); ++it)
    {
        if (it.value() < min)
        {
            min = it.value();
            thread = it.key();
        }
    }
    return thread;
}

void SocksProxyConnectionManager::addConnectionToThread(QThread *thread, SocksProxyConnection *connection)
{
    //qCDebug(LOG_SOCKS_SERVER) << "Connection started:" << connection;
    connection->moveToThread(thread);
    QTimer::singleShot(0, connection, SLOT(start()));
    WS_ASSERT(!connections_.contains(connection));
    connections_[connection] = thread;
    threads_[thread]++;
}

} // namespace SocksProxyServer
