#include "socksproxyserver.h"
#include "utils/ws_assert.h"
#include "utils/log/categories.h"

namespace SocksProxyServer {

SocksProxyServer::SocksProxyServer(QObject *parent) : QTcpServer(parent)
{
    usersCounter_ = new ConnectedUsersCounter(this);
    connect(usersCounter_, &ConnectedUsersCounter::usersCountChanged, this, &SocksProxyServer::usersCountChanged);
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
        qCInfo(LOG_SOCKS_SERVER) << "Socks proxy server started on port" << serverPort();
        return true;
    }
    else
    {
        qCCritical(LOG_SOCKS_SERVER) << "Can't start socks proxy server on port" << port;
        return false;
    }
}

void SocksProxyServer::stopServer()
{
    if (isListening())
    {
        qCInfo(LOG_SOCKS_SERVER) << "Socks proxy server stopped on port" << serverPort();
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
