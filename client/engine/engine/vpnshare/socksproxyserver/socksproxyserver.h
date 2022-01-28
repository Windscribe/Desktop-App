#ifndef SOCKSPROXYSERVER_H
#define SOCKSPROXYSERVER_H

#include "socksproxyconnectionmanager.h"
#include "../connecteduserscounter.h"

#include <QTcpServer>

namespace SocksProxyServer {

class SocksProxyServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit SocksProxyServer(QObject *parent);
    virtual ~SocksProxyServer();

    bool startServer(quint16 port);
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
};


} // namespace SocksProxyServer

#endif // SOCKSPROXYSERVER_H
