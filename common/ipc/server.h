#ifndef IPCSERVER_H
#define IPCSERVER_H

#include "iserver.h"
#include <QLocalServer>

namespace IPC
{

class Server : public QObject, public IServer
{
    Q_OBJECT
public:
    Server();
    virtual ~Server();

    virtual bool start();

signals:
    void newConnection(IPC::IConnection *connection);

private slots:
    void onNewConnection();

private:
    QLocalServer server_;
};

} // namespace IPC

#endif // IPCSERVER_H
