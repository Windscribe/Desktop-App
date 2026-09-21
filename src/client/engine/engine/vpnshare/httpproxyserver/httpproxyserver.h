#pragma once

#include "httpproxyconnectionmanager.h"
#include "../connecteduserscounter.h"
#include "../proxyauthconfig.h"
#include "../proxyconnectionmanager.h"  // for base class visibility in tests

#include <QHostAddress>
#include <QTcpServer>

class TestProxyServers;

namespace HttpProxyServer {

class HttpProxyServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit HttpProxyServer(QObject *parent);
    virtual ~HttpProxyServer();

    bool startServer(const QHostAddress &bindAddress, int prefixLength, quint16 port, const ProxyAuth::Config &auth);

    int getConnectedUsersCount();

    void closeActiveConnections();

signals:
    void usersCountChanged();

protected:
    virtual void incomingConnection(qintptr socketDescriptor);

private:
    HttpProxyConnectionManager *connectionManager_;
    ConnectedUsersCounter *usersCounter_;
    ProxyAuth::Config auth_;
    QHostAddress bindAddress_;
    int prefixLength_ = 0;
    // Bounds what peers can pin in this process; every accepted connection holds at least one descriptor.
    static int maxConnections_;
    // One warning per rejection episode and reason, cleared by the next accepted connection; a rejected peer can retry
    // at line rate and would otherwise flood the log.
    bool offSubnetLogged_ = false;
    bool limitLogged_ = false;

    // Terminal: the worker threads are not restarted, so this only runs from the destructor.
    void stopServer();

    friend class ::TestProxyServers;
};

} // namespace HttpProxyServer
