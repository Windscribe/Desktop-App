#include "socketwriteall.h"

SocketWriteAll::SocketWriteAll(QObject *parent, QTcpSocket *socket) : QObject(parent),
    socket_(socket), bEmitAllDataWritten_(false)
{
    connect(socket_, SIGNAL(bytesWritten(qint64)), SLOT(onBytesWritten(qint64)));
}

void SocketWriteAll::write(const QByteArray &arr)
{
    if (arr_.isEmpty())
    {
        arr_ = arr;
        socket_->write(arr_);
    }
    else
    {
        arr_.append(arr);
    }
}

void SocketWriteAll::setEmitAllDataWritten()
{
    if (arr_.isEmpty())
    {
        emit allDataWriteFinished();
    }
    bEmitAllDataWritten_ = true;
}

void SocketWriteAll::onBytesWritten(qint64 bytes)
{
    arr_.remove(0, bytes);
    if (!arr_.isEmpty())
    {
        socket_->write(arr_);
    }
    else
    {
        if (bEmitAllDataWritten_)
        {
            emit allDataWriteFinished();
        }
    }
}
