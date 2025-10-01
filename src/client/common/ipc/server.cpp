#include "server.h"
#include "connection.h"
#include "utils/log/categories.h"
#include <QDir>

namespace IPC
{

Server::Server()
{
    connect(&server_, &QLocalServer::newConnection, this, &Server::onNewConnection);
}

Server::~Server()
{
}

bool Server::start()
{

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    // remove socket file, if already exists (for Mac/Linux)
    QString connectingPathName = QDir::tempPath();
    QFile::remove("/var/run/windscribe/localipc.sock");

    bool b = server_.listen("/var/run/windscribe/localipc.sock");
#else
    bool b = server_.listen("Windscribe8rM7bza5OR");
#endif

    if (!b)
        qCCritical(LOG_IPC) << "IPC server listen error:" << server_.errorString();
    return b;
}


void Server::onNewConnection()
{
    while (server_.hasPendingConnections())
    {
        QLocalSocket *socket = server_.nextPendingConnection();
        Connection *connection = new Connection(socket);
        emit newConnection(connection);
    }
}

} // namespace IPC
