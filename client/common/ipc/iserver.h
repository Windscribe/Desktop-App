#ifndef IIPCSERVER_H
#define IIPCSERVER_H

#include <QObject>
#include "iconnection.h"

namespace IPC
{

// Interprocess communication interface (for server)
class IServer
{
public:
    virtual ~IServer() {}
    virtual bool start() = 0;

signals:
    virtual void newConnection(IPC::IConnection * connection) = 0;
};

} // namespace IPC

#endif // IIPCSERVER_H
