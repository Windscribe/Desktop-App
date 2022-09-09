#include "httpproxyserver.h"
#include "utils/ws_assert.h"
#include "utils/logger.h"

namespace HttpProxyServer {

HttpProxyServer::HttpProxyServer(QObject *parent) : QTcpServer(parent)
{
    usersCounter_ = new ConnectedUsersCounter(this);
    connect(usersCounter_, SIGNAL(usersCountChanged()), SIGNAL(usersCountChanged()));
    connectionManager_ = new HttpProxyConnectionManager(this, 4, usersCounter_);
}

HttpProxyServer::~HttpProxyServer()
{
    stopServer();
}

bool HttpProxyServer::startServer(quint16 port)
{
    WS_ASSERT(!isListening());

    if (listen(QHostAddress::AnyIPv4, port))
    {
        qCDebug(LOG_HTTP_SERVER) << "Http proxy server started on port" << serverPort();
        return true;
    }
    else
    {
        qCDebug(LOG_HTTP_SERVER) << "Can't start http proxy server on port" << port;
        return false;
    }
}

void HttpProxyServer::stopServer()
{
    if (isListening())
    {
        qCDebug(LOG_HTTP_SERVER) << "Http proxy server stopped on port" << serverPort();
        close();
    }
    connectionManager_->stop();
    usersCounter_->reset();
}

int HttpProxyServer::getConnectedUsersCount()
{
    return usersCounter_->getConnectedUsersCount();
}

void HttpProxyServer::closeActiveConnections()
{
    connectionManager_->closeAllConnections();
}

void HttpProxyServer::incomingConnection(qintptr socketDescriptor)
{
    connectionManager_->newConnection(socketDescriptor);
}


} // namespace HttpProxyServer
