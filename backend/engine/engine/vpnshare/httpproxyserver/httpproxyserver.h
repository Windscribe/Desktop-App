#ifndef HTTPPROXYSERVER_H
#define HTTPPROXYSERVER_H

#include "httpproxyconnectionmanager.h"
#include "../connecteduserscounter.h"

#include <QTcpServer>

namespace HttpProxyServer {

class HttpProxyServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit HttpProxyServer(QObject *parent);
    virtual ~HttpProxyServer();

    bool startServer(quint16 port);
    void stopServer();

    int getConnectedUsersCount();

    void closeActiveConnections();

signals:
    void usersCountChanged();

protected:
    virtual void incomingConnection(qintptr socketDescriptor);

private:
    HttpProxyConnectionManager *connectionManager_;
    ConnectedUsersCounter *usersCounter_;
};


} // namespace HttpProxyServer

#endif // HTTPPROXYSERVER_H
