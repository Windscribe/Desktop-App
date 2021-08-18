#ifndef SOCKSPROXYCONNECTIONMANAGER_H
#define SOCKSPROXYCONNECTIONMANAGER_H

#include <QObject>
#include <QMap>
#include "socksproxyconnection.h"
#include "../connecteduserscounter.h"

namespace SocksProxyServer {

class SocksProxyConnectionManager : public QObject
{
    Q_OBJECT
public:
    explicit SocksProxyConnectionManager(QObject *parent, int threadsCount, ConnectedUsersCounter *usersCounter);

public:
    void newConnection(qintptr socketDescriptor);
    void closeAllConnections();
    void stop();

private slots:
    void onConnectionFinished(const QString &hostname);

private:
    QMap<QThread *, quint32> threads_;
    QMap<SocksProxyConnection *, QThread *> connections_;
    ConnectedUsersCounter *usersCounter_;

    QThread *getLessBusyThread();
    void addConnectionToThread(QThread *thread, SocksProxyConnection *connection);
};

} // namespace SocksProxyServer

#endif // SOCKSPROXYCONNECTIONMANAGER_H
