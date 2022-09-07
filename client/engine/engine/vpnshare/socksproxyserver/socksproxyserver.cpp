#include "socksproxyserver.h"
#include "utils/ws_assert.h"
#include "utils/logger.h"

namespace SocksProxyServer {

SocksProxyServer::SocksProxyServer(QObject *parent) : QTcpServer(parent)
{
    usersCounter_ = new ConnectedUsersCounter(this);
    connect(usersCounter_, SIGNAL(usersCountChanged()), SIGNAL(usersCountChanged()));
    connectionManager_ = new SocksProxyConnectionManager(this, 4, usersCounter_);
}

SocksProxyServer::~SocksProxyServer()
{
    stopServer();
}

bool SocksProxyServer::startServer(quint16 port)
{
    WS_ASSERT(!isListening());

    if (listen(QHostAddress::AnyIPv4, port))
    {
        qCDebug(LOG_SOCKS_SERVER) << "Socks proxy server started on port" << serverPort();
        return true;
    }
    else
    {
        qCDebug(LOG_SOCKS_SERVER) << "Can't start socks proxy server on port" << port;
        return false;
    }
}

void SocksProxyServer::stopServer()
{
    if (isListening())
    {
        qCDebug(LOG_SOCKS_SERVER) << "Socks proxy server stopped on port" << serverPort();
        close();
    }
    connectionManager_->stop();
}

int SocksProxyServer::getConnectedUsersCount()
{
    return usersCounter_->getConnectedUsersCount();
}

void SocksProxyServer::closeActiveConnections()
{
    connectionManager_->closeAllConnections();
}

void SocksProxyServer::incomingConnection(qintptr socketDescriptor)
{
    connectionManager_->newConnection(socketDescriptor);
}


} // namespace SocksProxyServer
