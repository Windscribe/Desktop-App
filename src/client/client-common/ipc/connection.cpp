#include "connection.h"
#include "commandfactory.h"
#include <QStandardPaths>
#include <QTimer>
#include "utils/log/categories.h"
#include "utils/ws_assert.h"

namespace IPC
{

Connection::Connection(QLocalSocket *localSocket) : localSocket_(localSocket), bytesWrittingInProgress_(0)
{
    QObject::connect(localSocket_, &QLocalSocket::disconnected, this, &Connection::onSocketDisconnected);
    QObject::connect(localSocket_, &QLocalSocket::bytesWritten, this, &Connection::onSocketBytesWritten);
    QObject::connect(localSocket_, &QLocalSocket::readyRead, this, &Connection::onReadyRead);
    QObject::connect(localSocket_, &QLocalSocket::errorOccurred, this, &Connection::onSocketError);
}

Connection::Connection() : localSocket_(NULL), bytesWrittingInProgress_(0)
{
}

Connection::~Connection()
{
    safeDeleteSocket();
}

void Connection::connect()
{
    safeDeleteSocket();
    localSocket_ = new QLocalSocket;
    QObject::connect(localSocket_, &QLocalSocket::connected, this, &Connection::onSocketConnected);
    QObject::connect(localSocket_, &QLocalSocket::disconnected, this, &Connection::onSocketDisconnected);
    QObject::connect(localSocket_, &QLocalSocket::bytesWritten, this, &Connection::onSocketBytesWritten);
    QObject::connect(localSocket_, &QLocalSocket::readyRead, this, &Connection::onReadyRead);
    QObject::connect(localSocket_, &QLocalSocket::errorOccurred, this, &Connection::onSocketError);
#if defined(Q_OS_MACOS)
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
        "/" WS_SETTINGS_ORG "/ipc";
    localSocket_->connectToServer(runtimeDir + "/" WS_PRODUCT_NAME_LOWER "-localipc.sock");
#elif defined(Q_OS_LINUX)
    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    localSocket_->connectToServer(runtimeDir + "/" WS_PRODUCT_NAME_LOWER "-localipc.sock");
#else
    localSocket_->connectToServer(WS_APP_IDENTIFIER "8rM7bza5OR");
#endif
}

void Connection::close()
{
    WS_ASSERT(localSocket_ != NULL);
    safeDeleteSocket();
}

void Connection::sendCommand(const Command &commandl)
{
    WS_ASSERT(localSocket_ != NULL);

    // command structure
    // 1) (int) size of protobuf message in bytes
    // 2) (int) size of message string id in bytes
    // 3) (string) message string id
    // 4) (byte array) body of protobuf message

    std::vector<char> buf = commandl.getData();
    int sizeOfBuf = buf.size();
    std::string strId = commandl.getStringId();
    int sizeOfStringId = strId.length();

    WS_ASSERT(sizeOfStringId > 0);

    bool isWriteBufIsEmpty = writeBuf_.isEmpty();

    writeBuf_.append((const char *)&sizeOfBuf, sizeof(sizeOfBuf));
    writeBuf_.append((const char *)&sizeOfStringId, sizeof(sizeOfStringId));
    writeBuf_.append(strId.c_str(), sizeOfStringId);
    if (sizeOfBuf > 0)
    {
        writeBuf_.append(&buf[0], sizeOfBuf);
    }
    if (isWriteBufIsEmpty)
    {
        qint64 bytesWritten = localSocket_->write(writeBuf_);
        if (bytesWritten == -1)
        {
            emit stateChanged(CONNECTION_DISCONNECTED, this);
        }
        else
        {
            bytesWrittingInProgress_ += bytesWritten;
            writeBuf_.remove(0, bytesWritten);
        }
    }
}

void Connection::onSocketConnected()
{
    emit stateChanged(CONNECTION_CONNECTED, this);
}

void Connection::onSocketDisconnected()
{
    // qDebug() << "Socket disconnected";

    // Only emit disconnect if socket has not already errored out:
    // * On MacOS socket will error and then disconnect when Engine is forcibly terminated
    // * The "duplicate" state changes prevent the gui from waiting for engine recovery and thus tear down the entire application
    // * This doesn't occur on Windows as forcibly killing the engine doesn't trigger socket error
    if (localSocket_)
    {
        if (localSocket_->error() == QLocalSocket::UnknownSocketError)
        {
            emit stateChanged(CONNECTION_DISCONNECTED, this);
        }
    }
    else
    {
        // user manually closes app -- local socket is cleaned up by close()
        emit stateChanged(CONNECTION_DISCONNECTED, this);
    }
}

void Connection::onSocketBytesWritten(qint64 bytes)
{
    bytesWrittingInProgress_ -= bytes;

    if (!writeBuf_.isEmpty())
    {
        qint64 bytesWritten = localSocket_->write(writeBuf_);
        if (bytesWritten == -1)
        {
            emit stateChanged(CONNECTION_DISCONNECTED, this);
        }
        else
        {
            bytesWrittingInProgress_ += bytesWritten;
            writeBuf_.remove(0, bytesWritten);
        }
    }
    else if (bytesWrittingInProgress_ == 0)
    {
        emit allWritten(this);
    }
}

void Connection::onReadyRead()
{
    readBuf_.append(localSocket_->readAll());
    while (canReadCommand())
    {
        Command *cmd = readCommand();
        // An unrecognized command id yields a null command; readCommand still consumes the frame,
        // so drop it here rather than forwarding a null pointer to the newCommand slots.
        if (cmd != nullptr) {
            emit newCommand(cmd, this);
        }
    }
}

void Connection::onSocketError(QLocalSocket::LocalSocketError socketError)
{
    Q_UNUSED(socketError)
    // qDebug() << "Socket error: " << socketError;
    if (socketError == QLocalSocket::LocalSocketError::PeerClosedError) {
        emit stateChanged(CONNECTION_DISCONNECTED, this);
    } else {
        emit stateChanged(CONNECTION_ERROR, this);
    }
}

// Maximum allowed size for individual IPC frame fields.
// 10 MB is generous for any legitimate protobuf message or command ID.
static constexpr int kMaxIpcFieldSize = 10 * 1024 * 1024;

bool Connection::canReadCommand()
{
    if (readBuf_.size() > (int)(sizeof(int) * 2))
    {
        int sizeOfCmd;
        int sizeOfId;
        memcpy(&sizeOfCmd, readBuf_.data(), sizeof(int));
        memcpy(&sizeOfId, readBuf_.data() + sizeof(int), sizeof(int));

        // Reject negative or oversized lengths
        if (sizeOfCmd < 0 || sizeOfId <= 0 || sizeOfCmd > kMaxIpcFieldSize || sizeOfId > kMaxIpcFieldSize) {
            qCWarning(LOG_IPC) << "IPC frame with invalid lengths: cmd=" << sizeOfCmd << "id=" << sizeOfId;
            readBuf_.clear();
            return false;
        }

        if (readBuf_.size() >= (int)(sizeof(int) * 2 + sizeOfCmd + sizeOfId))
        {
            return true;
        }
    }
    return false;
}

Command *Connection::readCommand()
{
    int sizeOfCmd;
    int sizeOfId;
    memcpy(&sizeOfCmd, readBuf_.data(), sizeof(int));
    memcpy(&sizeOfId, readBuf_.data() + sizeof(int), sizeof(int));

    std::string strId(readBuf_.data() + sizeof(int) * 2, sizeOfId);

    Command *cmd = CommandFactory::makeCommand(strId, readBuf_.data() + sizeof(int) * 2 + sizeOfId, sizeOfCmd);
    if (cmd == nullptr) {
        qCWarning(LOG_IPC) << "Dropping IPC command with unrecognized id:"
                           << QString::fromStdString(strId.substr(0, 64)) << "(id length" << sizeOfId << ")";
    }
    readBuf_.remove(0, sizeof(int) * 2 + sizeOfId + sizeOfCmd);
    return cmd;
}

void Connection::safeDeleteSocket()
{
    if (localSocket_)
    {
        localSocket_->deleteLater();
        localSocket_ = NULL;
    }
}

} // namespace IPC
