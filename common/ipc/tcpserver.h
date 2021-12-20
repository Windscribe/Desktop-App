#ifndef IPCTCPSERVER_H
#define IPCTCPSERVER_H

#include "iserver.h"
#include <QTcpServer>

namespace IPC
{

class TcpServer : public QObject, public IServer
{
    Q_OBJECT
public:
    TcpServer();
    ~TcpServer() override;

    bool start() override;

private slots:
    void onNewConnection();

private:
    QTcpServer server_;
};

} // namespace IPC

#endif // IPCTCPSERVER_H
