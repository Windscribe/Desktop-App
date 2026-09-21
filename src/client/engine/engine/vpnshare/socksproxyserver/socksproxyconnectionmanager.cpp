#include "socksproxyconnectionmanager.h"

namespace SocksProxyServer {

SocksProxyConnectionManager::SocksProxyConnectionManager(QObject *parent, int threadsCount, ConnectedUsersCounter *usersCounter)
    : ProxyServer::ProxyConnectionManager(parent, threadsCount, usersCounter)
{
}

void SocksProxyConnectionManager::newConnection(qintptr socketDescriptor, const QString &peer, const ProxyAuth::Config &auth)
{
    SocksProxyConnection *connection = new SocksProxyConnection(socketDescriptor, peer, auth);
    connect(connection, &SocksProxyConnection::finished, this, &SocksProxyConnectionManager::onConnectionFinished);
    newConnectionBase(peer, connection);
}

void SocksProxyConnectionManager::onConnectionFinished(const QString &hostname)
{
    ProxyServer::ProxyConnectionManager::handleConnectionFinished(hostname);
}

} // namespace SocksProxyServer
