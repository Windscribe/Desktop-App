#include "tcpserver.h"
#include "tcpconnection.h"
#include <QDir>

namespace IPC
{

TcpServer::TcpServer()
{
    connect(&server_, SIGNAL(newConnection()), SLOT(onNewConnection()));
}

TcpServer::~TcpServer()
{
}

bool TcpServer::start()
{
    bool b = server_.listen(QHostAddress::Any, 12345);
    return b;
}

void TcpServer::onNewConnection()
{
    while (server_.hasPendingConnections())
    {
        QTcpSocket *socket = server_.nextPendingConnection();
        IConnection *connection = new TcpConnection(socket);
        emit newConnection(connection);
    }
}

} // namespace IPC
