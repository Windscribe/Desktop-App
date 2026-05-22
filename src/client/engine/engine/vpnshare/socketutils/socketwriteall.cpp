#include "socketwriteall.h"

SocketWriteAll::SocketWriteAll(QObject *parent, QTcpSocket *socket) : QObject(parent),
    socket_(socket), bEmitAllDataWritten_(false)
{
    connect(socket_, &QTcpSocket::bytesWritten, this, &SocketWriteAll::onBytesWritten);
}

void SocketWriteAll::write(const QByteArray &arr)
{
    if (arr.isEmpty()) {
        return;
    }
    const qint64 n = socket_->write(arr);
    if (n < 0) {
        return;
    }
    Q_ASSERT(n == arr.size());
}

void SocketWriteAll::setEmitAllDataWritten()
{
    bEmitAllDataWritten_ = true;
    if (socket_->bytesToWrite() == 0) {
        emit allDataWriteFinished();
    }
}

void SocketWriteAll::onBytesWritten(qint64 /*bytes*/)
{
    if (bEmitAllDataWritten_ && socket_->bytesToWrite() == 0) {
        emit allDataWriteFinished();
    }
}
