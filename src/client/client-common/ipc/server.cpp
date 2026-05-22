#include "server.h"
#include "connection.h"
#include "utils/log/categories.h"
#include <QFile>
#include <QStandardPaths>

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

#if defined(Q_OS_MACOS)
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString path = runtimeDir + "/windscribe-localipc.sock";
    QFile::remove(path);
    server_.setSocketOptions(QLocalServer::UserAccessOption);
    bool b = server_.listen(path);
#elif defined(Q_OS_LINUX)
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    const QString path = runtimeDir + "/windscribe-localipc.sock";
    QFile::remove(path);
    server_.setSocketOptions(QLocalServer::UserAccessOption);
    bool b = server_.listen(path);
#else
    bool b = server_.listen(WS_APP_IDENTIFIER "8rM7bza5OR");
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
