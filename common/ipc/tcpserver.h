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
    virtual ~TcpServer();

    virtual bool start();

private slots:
    void onNewConnection();

private:
    QTcpServer server_;
};

} // namespace IPC

#endif // IPCTCPSERVER_H
