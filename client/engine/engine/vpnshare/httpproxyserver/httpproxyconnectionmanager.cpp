#include "httpproxyconnectionmanager.h"

#include <QThread>
#include <QTimer>

namespace HttpProxyServer {

HttpProxyConnectionManager::HttpProxyConnectionManager(QObject *parent, int threadsCount, ConnectedUsersCounter *usersCounter) : QObject(parent),
    usersCounter_(usersCounter)
{
    Q_ASSERT(threadsCount > 0);
    for (int i = 0; i < threadsCount; i++)
    {
        QThread *thread = new QThread(this);
        threads_[thread] = 0;
        thread->start(QThread::LowPriority);
    }
}

void HttpProxyConnectionManager::newConnection(qintptr socketDescriptor)
{
#ifdef Q_OS_WIN
    SOCKADDR_IN  addr = {0};
    int addr_len = sizeof(addr);
#elif defined (Q_OS_MAC) || defined (Q_OS_LINUX)
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
#endif
    getpeername(socketDescriptor, (sockaddr*)&addr, &addr_len);
    char *ip = inet_ntoa(addr.sin_addr);
    usersCounter_->newUserConnected(ip);
    QThread *thread = getLessBusyThread();
    HttpProxyConnection *connection = new HttpProxyConnection(socketDescriptor, ip);
    connect(connection, SIGNAL(finished(QString)), SLOT(onConnectionFinished(QString)));
    addConnectionToThread(thread, connection);

    //qDebug() << "Count of connections:" << connections_.count();
}

void HttpProxyConnectionManager::closeAllConnections()
{
     for(auto c : connections_.keys())
     {
         QMetaObject::invokeMethod(c, "forceClose", Qt::QueuedConnection);
     }
}

void HttpProxyConnectionManager::stop()
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

void HttpProxyConnectionManager::onConnectionFinished(const QString &hostname)
{
    HttpProxyConnection *connection = static_cast<HttpProxyConnection *>(sender());
    usersCounter_->userDiconnected(hostname);
    //qDebug() << "Connection finished:" << connection;
    QMap<HttpProxyConnection *, QThread *>::iterator it = connections_.find(connection);
    Q_ASSERT(it != connections_.end());

    QMap<QThread *, quint32>::iterator itThread = threads_.find(it.value());
    Q_ASSERT(itThread != threads_.end());
    itThread.value()--;

    it.key()->deleteLater();
    connections_.erase(it);

    //qDebug() << "Count of connections:" << connections_.count();

}

QThread *HttpProxyConnectionManager::getLessBusyThread()
{
    Q_ASSERT(threads_.count() > 0);
    quint32 min = threads_.begin().value();
    QThread *thread = threads_.begin().key();
    QMap<QThread *, quint32>::iterator it = threads_.begin();
    ++it;
    for (; it != threads_.end(); ++it)
    {
        if (it.value() < min)
        {
            min = it.value();
            thread = it.key();
        }
    }
    return thread;
}

void HttpProxyConnectionManager::addConnectionToThread(QThread *thread, HttpProxyConnection *connection)
{
    //qDebug() << "Connection started:" << connection;
    connection->moveToThread(thread);
    QTimer::singleShot(0, connection, SLOT(start()));
    Q_ASSERT(!connections_.contains(connection));
    connections_[connection] = thread;
    threads_[thread]++;
}

void HttpProxyConnectionManager::dumpThreads()
{
    for (QMap<QThread *, quint32>::iterator it = threads_.begin(); it != threads_.end(); ++it)
    {
        qDebug() << "Thread:" << it.key() << "Value:" << it.value();
    }
}

} // namespace HttpProxyServer
