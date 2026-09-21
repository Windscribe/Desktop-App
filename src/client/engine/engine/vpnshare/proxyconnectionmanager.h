#pragma once

#include <QMap>
#include <QObject>
#include <QThread>

#include "connecteduserscounter.h"

class TestProxyServers;

namespace ProxyServer {

// Common thread pool and connection lifecycle for both proxy servers.
class ProxyConnectionManager : public QObject
{
    Q_OBJECT
public:
    explicit ProxyConnectionManager(QObject *parent, int threadsCount, ConnectedUsersCounter *usersCounter);

    void closeAllConnections();
    void stop();
    int connectionCount() const { return static_cast<int>(connections_.size()); }

protected:
    void newConnectionBase(const QString &peer, QObject *connection);
    void handleConnectionFinished(const QString &hostname);

private:
    friend class ::TestProxyServers;

    QMap<QThread *, quint32> threads_;
    QMap<QObject *, QThread *> connections_;
    ConnectedUsersCounter *usersCounter_ = nullptr;

    QThread *getLessBusyThread();
    void addConnectionToThread(QThread *thread, QObject *connection);
};

} // namespace ProxyServer
