#include "server.h"
#include "connection.h"
#include "utils/logger.h"
#include <QDir>

namespace IPC
{

Server::Server()
{
    connect(&server_, SIGNAL(newConnection()), SLOT(onNewConnection()));
}

Server::~Server()
{
}

bool Server::start()
{

#ifdef Q_OS_MAC
    // remove socket file, if already exists (for Mac)
    QString connectingPathName = QDir::tempPath();
    connectingPathName += QLatin1Char('/') + "Windscribe8rM7bza5OR";
    QFile::remove(connectingPathName);
#endif

    server_.setSocketOptions(QLocalServer::UserAccessOption); // so user or admin CLI can talk to admin Engine (Launcher starts app in admin mode)
    bool b = server_.listen("Windscribe8rM7bza5OR");
    if (!b)
        qCDebug(LOG_IPC) << "IPC server listen error:" << server_.errorString();
    return b;
}


void Server::onNewConnection()
{
    while (server_.hasPendingConnections())
    {
        QLocalSocket *socket = server_.nextPendingConnection();
        IConnection *connection = new Connection(socket);
        emit newConnection(connection);
    }
}

} // namespace IPC
