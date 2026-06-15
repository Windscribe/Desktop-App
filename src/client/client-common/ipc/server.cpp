#include "server.h"
#include "connection.h"
#include "utils/log/categories.h"
#include <QDir>
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
    server_.setSocketOptions(QLocalServer::UserAccessOption);

#if defined(Q_OS_MACOS)
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
        "/" WS_SETTINGS_ORG "/ipc";
    QDir runtimeDirObj(runtimeDir);
    if (!runtimeDirObj.mkpath(".")) {
        qCCritical(LOG_IPC) << "IPC server could not create runtime directory:" << runtimeDir;
        return false;
    }
    QFile::setPermissions(runtimeDir,
        QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    const QString path = runtimeDir + "/" WS_PRODUCT_NAME_LOWER "-localipc.sock";
    QFile::remove(path);
    bool b = server_.listen(path);
#elif defined(Q_OS_LINUX)
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    const QString path = runtimeDir + "/" WS_PRODUCT_NAME_LOWER "-localipc.sock";
    QFile::remove(path);
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
