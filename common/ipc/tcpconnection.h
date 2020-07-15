#ifndef IPCTCPCONNECTION_H
#define IPCTCPCONNECTION_H

#include <QTcpSocket>
#include <QObject>
#include "iserver.h"

namespace IPC
{

class TcpConnection : public QObject, public IConnection
{
    Q_OBJECT
public:
    explicit TcpConnection(QTcpSocket *socket);
    explicit TcpConnection();
    virtual ~TcpConnection();

    virtual void connect();
    virtual void close();
    virtual void sendCommand(const Command &commandl);

signals:
    void newCommand(Command *cmd, IConnection *connection);
    void stateChanged(int state, IConnection *connection);
    void allWritten(IPC::IConnection *connection);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketBytesWritten(qint64 bytes);
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket *socket_;

    QByteArray writeBuf_;
    QByteArray readBuf_;

    bool canReadCommand();
    Command *readCommand();

    void safeDeleteSocket();
};

} // namespace IPC

#endif // IPCTCPCONNECTION_H
