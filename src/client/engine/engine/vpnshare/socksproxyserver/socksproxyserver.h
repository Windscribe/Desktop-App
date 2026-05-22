#pragma once

#include "socksproxyconnectionmanager.h"
#include "../connecteduserscounter.h"
#include "../proxyauthconfig.h"

#include <QHostAddress>
#include <QTcpServer>

namespace SocksProxyServer {

class SocksProxyServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit SocksProxyServer(QObject *parent);
    virtual ~SocksProxyServer();

    bool startServer(const QHostAddress &bindAddress, int prefixLength, quint16 port, const ProxyAuth::Config &auth);
    void stopServer();

    int getConnectedUsersCount();

    void closeActiveConnections();

signals:
    void usersCountChanged();

protected:
    virtual void incomingConnection(qintptr socketDescriptor);


private:
    SocksProxyConnectionManager *connectionManager_;
    ConnectedUsersCounter *usersCounter_;
    ProxyAuth::Config auth_;
    QHostAddress bindAddress_;
    int prefixLength_ = 0;
};

} // namespace SocksProxyServer
