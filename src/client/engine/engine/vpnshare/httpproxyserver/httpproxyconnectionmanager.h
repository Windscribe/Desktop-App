#pragma once

#include "../proxyconnectionmanager.h"
#include "httpproxyconnection.h"

namespace HttpProxyServer {

class HttpProxyConnectionManager : public ProxyServer::ProxyConnectionManager
{
    Q_OBJECT
public:
    explicit HttpProxyConnectionManager(QObject *parent, int threadsCount, ConnectedUsersCounter *usersCounter);

    void newConnection(qintptr socketDescriptor, const QString &peer, const ProxyAuth::Config &auth);

private slots:
    void onConnectionFinished(const QString &hostname);

};

} // namespace HttpProxyServer
