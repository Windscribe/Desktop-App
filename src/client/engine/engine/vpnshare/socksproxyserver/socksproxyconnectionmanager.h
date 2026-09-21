#pragma once

#include "../proxyconnectionmanager.h"
#include "socksproxyconnection.h"

namespace SocksProxyServer {

class SocksProxyConnectionManager : public ProxyServer::ProxyConnectionManager
{
    Q_OBJECT
public:
    explicit SocksProxyConnectionManager(QObject *parent, int threadsCount, ConnectedUsersCounter *usersCounter);

    void newConnection(qintptr socketDescriptor, const QString &peer, const ProxyAuth::Config &auth);

private slots:
    void onConnectionFinished(const QString &hostname);
};

} // namespace SocksProxyServer
