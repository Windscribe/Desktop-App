#include "localipcserver.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "ipc/server.h"

LocalIPCServer::LocalIPCServer(QObject *parent) : QObject(parent)
  , server_(NULL)

{

}

LocalIPCServer::~LocalIPCServer()
{
    SAFE_DELETE(server_);
    qCDebug(LOG_CLI_IPC) << "IPC server for CLI stopped";
}

void LocalIPCServer::start()
{
    Q_ASSERT(server_ == NULL);
    server_ = new IPC::Server();
    connect(dynamic_cast<QObject*>(server_), SIGNAL(newConnection(IPC::IConnection *)), SLOT(onServerCallbackAcceptFunction(IPC::IConnection *)), Qt::QueuedConnection);

    if (!server_->start())
    {
        qCDebug(LOG_CLI_IPC) << "Can't start IPC server for CLI";
    }
    else
    {
        qCDebug(LOG_CLI_IPC) << "IPC server for CLI started";
    }
}

void LocalIPCServer::onServerCallbackAcceptFunction(IPC::IConnection *connection)
{

}

void LocalIPCServer::onConnectionCommandCallback(IPC::Command *command, IPC::IConnection *connection)
{

}

void LocalIPCServer::onConnectionStateCallback(int state, IPC::IConnection *connection)
{

}
