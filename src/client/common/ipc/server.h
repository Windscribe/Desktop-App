#pragma once

#include <QLocalServer>
#include "connection.h"

namespace IPC
{

class Server : public QObject
{
    Q_OBJECT
public:
    Server();
    ~Server();

    bool start();

signals:
    void newConnection(IPC::Connection *connection);

private slots:
    void onNewConnection();

private:
    QLocalServer server_;
};

} // namespace IPC
