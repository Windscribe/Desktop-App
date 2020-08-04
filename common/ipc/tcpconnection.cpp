#include "tcpconnection.h"
#include "commandfactory.h"
#include "utils/logger.h"

namespace IPC
{

TcpConnection::TcpConnection(QTcpSocket *socket) : socket_(socket)
{
    QObject::connect(socket_, SIGNAL(disconnected()), SLOT(onSocketDisconnected()));
    QObject::connect(socket_, SIGNAL(bytesWritten(qint64)), SLOT(onSocketBytesWritten(qint64)));
    QObject::connect(socket_, SIGNAL(readyRead()), SLOT(onReadyRead()));
    QObject::connect(socket_, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(onSocketError(QAbstractSocket::SocketError)));
}

TcpConnection::TcpConnection() : socket_(NULL)
{
}

TcpConnection::~TcpConnection()
{
    safeDeleteSocket();
}

void TcpConnection::connect()
{
    Q_ASSERT(socket_ == NULL);
    socket_ = new QTcpSocket();
    QObject::connect(socket_, SIGNAL(connected()), SLOT(onSocketConnected()));
    QObject::connect(socket_, SIGNAL(disconnected()), SLOT(onSocketDisconnected()));
    QObject::connect(socket_, SIGNAL(bytesWritten(qint64)), SLOT(onSocketBytesWritten(qint64)));
    QObject::connect(socket_, SIGNAL(readyRead()), SLOT(onReadyRead()));
    QObject::connect(socket_, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(onSocketError(QAbstractSocket::SocketError)));
    //socket_->connectToHost("192.168.1.138", 12345);
    socket_->connectToHost("localhost", 12345);
}

void TcpConnection::close()
{
    Q_ASSERT(socket_ != NULL);
    socket_->close();
    safeDeleteSocket();
}

void TcpConnection::sendCommand(const Command &commandl)
{
    Q_ASSERT(socket_ != NULL);

    // command structure
    // 1) (int) size of protobuf message in bytes
    // 2) (int) size of message string id in bytes
    // 3) (string) message string id
    // 4) (byte array) body of protobuf message

    std::vector<char> buf = commandl.getData();
    int sizeOfBuf = buf.size();
    std::string strId = commandl.getStringId();
    int sizeOfStringId = strId.length();

    Q_ASSERT(sizeOfBuf > 0);
    Q_ASSERT(sizeOfStringId > 0);

    bool isWriteBufIsEmpty = writeBuf_.isEmpty();

    writeBuf_.append((const char *)&sizeOfBuf, sizeof(sizeOfBuf));
    writeBuf_.append((const char *)&sizeOfStringId, sizeof(sizeOfStringId));
    writeBuf_.append(strId.c_str(), sizeOfStringId);
    writeBuf_.append(&buf[0], sizeOfBuf);
    if (isWriteBufIsEmpty)
    {
        if (socket_->write(writeBuf_) == -1)
        {
            emit stateChanged(CONNECTION_DISCONNECTED, this);
            safeDeleteSocket();
        }
    }
}


void TcpConnection::onSocketConnected()
{
    emit stateChanged(CONNECTION_CONNECTED, this);
}

void TcpConnection::onSocketDisconnected()
{
    emit stateChanged(CONNECTION_DISCONNECTED, this);
    safeDeleteSocket();
}

void TcpConnection::onSocketBytesWritten(qint64 bytes)
{
    writeBuf_.remove(0, bytes);
    if (!writeBuf_.isEmpty())
    {
        if (socket_->write(writeBuf_) == -1)
        {
            emit stateChanged(CONNECTION_DISCONNECTED, this);
            safeDeleteSocket();
        }
    }
    else
    {
        emit allWritten(this);
    }
}

void TcpConnection::onReadyRead()
{
    readBuf_.append(socket_->readAll());
    while (canReadCommand())
    {
        Command *cmd = readCommand();
        emit newCommand(cmd, this);
    }
}

void TcpConnection::onSocketError(QAbstractSocket::SocketError /*socketError*/)
{
    qCDebug(LOG_CONNECTION) << "Socket error:" << socket_->errorString();
    safeDeleteSocket();
    emit stateChanged(CONNECTION_ERROR, this);
}

bool TcpConnection::canReadCommand()
{
    if (readBuf_.size() > (int)(sizeof(int) * 2))
    {
        int sizeOfCmd;
        int sizeOfId;
        memcpy(&sizeOfCmd, readBuf_.data(), sizeof(int));
        memcpy(&sizeOfId, readBuf_.data() + sizeof(int), sizeof(int));

        if (readBuf_.size() >= (int)(sizeof(int) * 2 + sizeOfCmd + sizeOfId))
        {
            return true;
        }
    }
    return false;
}

Command *TcpConnection::readCommand()
{
    int sizeOfCmd;
    int sizeOfId;
    memcpy(&sizeOfCmd, readBuf_.data(), sizeof(int));
    memcpy(&sizeOfId, readBuf_.data() + sizeof(int), sizeof(int));

    std::string strId(readBuf_.data() + sizeof(int) * 2, sizeOfId);

    Command *cmd = CommandFactory::makeCommand(strId, readBuf_.data() + sizeof(int) * 2 + sizeOfId, sizeOfCmd);
    readBuf_.remove(0, sizeof(int) * 2 + sizeOfId + sizeOfCmd);
    return cmd;
}

void TcpConnection::safeDeleteSocket()
{
    if (socket_)
    {
        socket_->deleteLater();
        socket_ = NULL;
    }
}

} // namespace IPC
