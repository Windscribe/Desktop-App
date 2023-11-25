#pragma once

#include <QLocalSocket>
#include <QObject>
#include "command.h"

namespace IPC
{

static const int CONNECTION_CONNECTED = 0;
static const int CONNECTION_DISCONNECTED = 1;
static const int CONNECTION_ERROR = 2;

class Connection : public QObject
{
    Q_OBJECT
public:
    explicit Connection(QLocalSocket *localSocket);
    explicit Connection();
    ~Connection();

    void connect();
    void close();
    void sendCommand(const Command &commandl);

signals:
    void newCommand(IPC::Command *cmd, IPC::Connection *connection);
    void stateChanged(int state, IPC::Connection *connection);
    void allWritten(IPC::Connection *connection);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketBytesWritten(qint64 bytes);
    void onReadyRead();
    void onSocketError(QLocalSocket::LocalSocketError socketError);

private:
    QLocalSocket *localSocket_;

    QByteArray writeBuf_;
    QByteArray readBuf_;
    qint64 bytesWrittingInProgress_;

    bool canReadCommand();
    Command *readCommand();

    void safeDeleteSocket();
};

} // namespace IPC
