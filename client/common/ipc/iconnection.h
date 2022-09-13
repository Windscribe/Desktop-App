#ifndef IIPCCONNECTION_H
#define IIPCCONNECTION_H

#include <QObject>
#include <functional>
#include "command.h"

namespace IPC
{

class IConnection;

static const int CONNECTION_CONNECTED = 0;
static const int CONNECTION_DISCONNECTED = 1;
static const int CONNECTION_ERROR = 2;

// connection between server and client
class IConnection
{
public:
    virtual ~IConnection() {}
    virtual void connect() = 0;
    virtual void close() = 0;
    virtual void sendCommand(const Command &command) = 0;

signals:
    virtual void newCommand(IPC::Command *cmd, IPC::IConnection *connection) = 0;
    virtual void stateChanged(int state, IPC::IConnection *connection) = 0;
    virtual void allWritten(IPC::IConnection *connection) = 0;

};

} // namespace IPC

#endif // IIPCCONNECTION_H
