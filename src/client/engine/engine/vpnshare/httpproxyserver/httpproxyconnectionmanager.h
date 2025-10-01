#pragma once

#include <QObject>
#include <QMap>
#include "httpproxyconnection.h"
#include "../connecteduserscounter.h"

namespace HttpProxyServer {

class HttpProxyConnectionManager : public QObject
{
    Q_OBJECT
public:
    explicit HttpProxyConnectionManager(QObject *parent, int threadsCount, ConnectedUsersCounter *usersCounter);

public:
    void newConnection(qintptr socketDescriptor);
    void closeAllConnections();
    void stop();

private slots:
    void onConnectionFinished(const QString &hostname);

private:
    QMap<QThread *, quint32> threads_;
    QMap<HttpProxyConnection *, QThread *> connections_;
    ConnectedUsersCounter *usersCounter_;

    QThread *getLessBusyThread();
    void addConnectionToThread(QThread *thread, HttpProxyConnection *connection);
    void dumpThreads();
};

} // namespace HttpProxyServer
