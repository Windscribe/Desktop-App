#include "proxyconnectionmanager.h"

#include <QTimer>
#include "utils/ws_assert.h"

namespace ProxyServer {

ProxyConnectionManager::ProxyConnectionManager(QObject *parent, int threadsCount, ConnectedUsersCounter *usersCounter)
    : QObject(parent), usersCounter_(usersCounter)
{
    WS_ASSERT(threadsCount > 0);
    WS_ASSERT(usersCounter != nullptr);
    for (int i = 0; i < threadsCount; ++i) {
        QThread *thread = new QThread(this);
        threads_[thread] = 0;
        thread->start(QThread::LowPriority);
    }
}

void ProxyConnectionManager::newConnectionBase(const QString &peer, QObject *connection)
{
    usersCounter_->newUserConnected(peer);

    QThread *thread = getLessBusyThread();
    addConnectionToThread(thread, connection);
}

void ProxyConnectionManager::closeAllConnections()
{
    for (auto c : connections_.keys()) {
        QMetaObject::invokeMethod(c, "forceClose", Qt::QueuedConnection);
    }
}

void ProxyConnectionManager::stop()
{
    // Connections are unparented and live in the worker threads. Close each in its own thread (which also cancels
    // an in-flight DNS lookup) and queue its deletion; a thread runs pending deletions as it finishes.
    for (auto it = connections_.cbegin(); it != connections_.cend(); ++it) {
        disconnect(it.key(), nullptr, this, nullptr);
        QMetaObject::invokeMethod(it.key(), "forceClose", Qt::BlockingQueuedConnection);
        it.key()->deleteLater();
    }
    connections_.clear();

    for (auto thread : threads_.keys()) {
        thread->exit();
    }
    for (auto thread : threads_.keys()) {
        thread->wait();
    }
}

void ProxyConnectionManager::handleConnectionFinished(const QString &hostname)
{
    usersCounter_->userDisconnected(hostname);
    QObject *connection = sender();
    auto it = connections_.find(connection);
    WS_ASSERT(it != connections_.end());
    auto threadIt = threads_.find(it.value());
    WS_ASSERT(threadIt != threads_.end());
    threadIt.value()--;
    connection->deleteLater();
    connections_.erase(it);
}

QThread *ProxyConnectionManager::getLessBusyThread()
{
    WS_ASSERT(threads_.count() > 0);
    quint32 min = threads_.begin().value();
    QThread *thread = threads_.begin().key();

    for (auto it = threads_.cbegin(); it != threads_.cend(); ++it) {
        if (it.value() < min) {
            min = it.value();
            thread = it.key();
        }
    }
    return thread;
}

void ProxyConnectionManager::addConnectionToThread(QThread *thread, QObject *connection)
{
    WS_ASSERT(thread->isRunning());
    connection->moveToThread(thread);
    QTimer::singleShot(0, connection, SLOT(start()));
    WS_ASSERT(!connections_.contains(connection));
    connections_[connection] = thread;
    threads_[thread]++;
}

} // namespace ProxyServer
