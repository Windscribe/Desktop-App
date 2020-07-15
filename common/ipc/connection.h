#ifndef IPCCONNECTION_H
#define IPCCONNECTION_H

#include <QLocalSocket>
#include <QObject>
#include "iserver.h"

namespace IPC
{

class Connection : public QObject, public IConnection
{
    Q_OBJECT
public:
    explicit Connection(QLocalSocket *localSocket);
    explicit Connection();
    virtual ~Connection();

    virtual void connect();
    virtual void close();
    virtual void sendCommand(const Command &commandl);

signals:
    void newCommand(IPC::Command *cmd, IPC::IConnection *connection);
    void stateChanged(int state, IPC::IConnection *connection);
    void allWritten(IPC::IConnection *connection);

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

#endif // IPCCONNECTION_H
