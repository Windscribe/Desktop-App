#include "httpproxyserver.h"
#include "../proxydestinationfilter.h"
#include "../socketutils/nativesocket.h"
#include "utils/log/categories.h"
#include "utils/ws_assert.h"

#ifdef Q_OS_WIN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

namespace HttpProxyServer {

int HttpProxyServer::maxConnections_ = 128;

HttpProxyServer::HttpProxyServer(QObject *parent) : QTcpServer(parent)
{
    usersCounter_ = new ConnectedUsersCounter(this);
    connect(usersCounter_, &ConnectedUsersCounter::usersCountChanged, this, &HttpProxyServer::usersCountChanged);
    connectionManager_ = new HttpProxyConnectionManager(this, 4, usersCounter_);
    // Qt stops accepting after a non-transient accept failure and never resumes on its own; the listener stays
    // bound but dead until this server is recreated, so make that state visible.
    connect(this, &QTcpServer::acceptError, this, [this](QAbstractSocket::SocketError error) {
        qCWarning(LOG_HTTP_SERVER) << "Accept failed, no longer accepting connections:" << error << errorString();
    });
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
        if (!offSubnetLogged_) {
            qCWarning(LOG_HTTP_SERVER) << "Rejecting off-subnet, non-private proxy peer" << peer.toString();
            offSubnetLogged_ = true;
        }
        SocketUtils::closeNativeSocket(socketDescriptor);
        return;
    }
    const QString peerAddress = peer.toString();
    if (connectionManager_->connectionCount() >= maxConnections_) {
        if (!limitLogged_) {
            qCWarning(LOG_HTTP_SERVER) << "Rejecting proxy peer" << peerAddress << ": connection limit reached";
            limitLogged_ = true;
        }
        SocketUtils::closeNativeSocket(socketDescriptor);
        return;
    }
    offSubnetLogged_ = false;
    limitLogged_ = false;
    connectionManager_->newConnection(socketDescriptor, peerAddress, auth_);
}


} // namespace HttpProxyServer
