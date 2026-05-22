#include "httpproxyserver.h"
#include "../proxydestinationfilter.h"
#include "utils/ws_assert.h"
#include "utils/log/categories.h"

#ifdef Q_OS_WIN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace HttpProxyServer {

HttpProxyServer::HttpProxyServer(QObject *parent) : QTcpServer(parent)
{
    usersCounter_ = new ConnectedUsersCounter(this);
    connect(usersCounter_, &ConnectedUsersCounter::usersCountChanged, this, &HttpProxyServer::usersCountChanged);
    connectionManager_ = new HttpProxyConnectionManager(this, 4, usersCounter_);
}

HttpProxyServer::~HttpProxyServer()
{
    stopServer();
}

bool HttpProxyServer::startServer(const QHostAddress &bindAddress, int prefixLength, quint16 port, const ProxyAuth::Config &auth)
{
    WS_ASSERT(!isListening());

    auth_ = auth;
    bindAddress_ = bindAddress;
    prefixLength_ = prefixLength;

    if (listen(bindAddress, port)) {
        qCInfo(LOG_HTTP_SERVER) << "Http proxy server started on" << bindAddress.toString() << "port" << serverPort();
        return true;
    } else {
        qCCritical(LOG_HTTP_SERVER) << "Can't start http proxy server on" << bindAddress.toString() << "port" << port;
        return false;
    }
}

void HttpProxyServer::stopServer()
{
    if (isListening()) {
        qCInfo(LOG_HTTP_SERVER) << "Http proxy server stopped on port" << serverPort();
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
    // Drop connections from non-private source IPs. Anything that arrives here from a public source must have crossed
    // the WAN; we never want to relay for those, even if the listener somehow ends up reachable from a public NIC.
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
#ifdef Q_OS_WIN
    int addrLen = sizeof(addr);
#else
    socklen_t addrLen = sizeof(addr);
#endif
    QHostAddress peer;
    if (getpeername(static_cast<int>(socketDescriptor), reinterpret_cast<sockaddr*>(&addr), &addrLen) == 0) {
        peer = QHostAddress(reinterpret_cast<sockaddr*>(&addr));
    }
    if (!ProxyDestinationFilter::isAllowedPeer(peer, bindAddress_, prefixLength_)) {
        qCWarning(LOG_HTTP_SERVER) << "Rejecting off-subnet, non-private proxy peer" << peer.toString();
#ifdef Q_OS_WIN
        ::closesocket(static_cast<SOCKET>(socketDescriptor));
#else
        ::close(static_cast<int>(socketDescriptor));
#endif
        return;
    }
    connectionManager_->newConnection(socketDescriptor, auth_);
}


} // namespace HttpProxyServer
