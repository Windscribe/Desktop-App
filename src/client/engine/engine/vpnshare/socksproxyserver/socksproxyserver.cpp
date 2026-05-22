#include "socksproxyserver.h"
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

bool SocksProxyServer::startServer(const QHostAddress &bindAddress, int prefixLength, quint16 port, const ProxyAuth::Config &auth)
{
    WS_ASSERT(!isListening());

    auth_ = auth;
    bindAddress_ = bindAddress;
    prefixLength_ = prefixLength;

    if (listen(bindAddress, port)) {
        qCInfo(LOG_SOCKS_SERVER) << "Socks proxy server started on" << bindAddress.toString() << "port" << serverPort();
        return true;
    } else {
        qCCritical(LOG_SOCKS_SERVER) << "Can't start socks proxy server on" << bindAddress.toString() << "port" << port;
        return false;
    }
}

void SocksProxyServer::stopServer()
{
    if (isListening()) {
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
        qCWarning(LOG_SOCKS_SERVER) << "Rejecting off-subnet, non-private proxy peer" << peer.toString();
#ifdef Q_OS_WIN
        ::closesocket(static_cast<SOCKET>(socketDescriptor));
#else
        ::close(static_cast<int>(socketDescriptor));
#endif
        return;
    }
    connectionManager_->newConnection(socketDescriptor, auth_);
}


} // namespace SocksProxyServer
