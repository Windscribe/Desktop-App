#include "httpproxyconnectionmanager.h"

namespace HttpProxyServer {

HttpProxyConnectionManager::HttpProxyConnectionManager(QObject *parent, int threadsCount, ConnectedUsersCounter *usersCounter)
    : ProxyServer::ProxyConnectionManager(parent, threadsCount, usersCounter)
{
}

void HttpProxyConnectionManager::newConnection(qintptr socketDescriptor, const QString &peer, const ProxyAuth::Config &auth)
{
    HttpProxyConnection *connection = new HttpProxyConnection(socketDescriptor, peer, auth);
    connect(connection, &HttpProxyConnection::finished, this, &HttpProxyConnectionManager::onConnectionFinished);
    newConnectionBase(peer, connection);
}

void HttpProxyConnectionManager::onConnectionFinished(const QString &hostname)
{
    ProxyServer::ProxyConnectionManager::handleConnectionFinished(hostname);
}

} // namespace HttpProxyServer
