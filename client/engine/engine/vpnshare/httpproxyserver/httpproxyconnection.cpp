#include "httpproxyconnection.h"
#include <QThread>
#include <QHostAddress>
#include "utils/logger.h"

namespace HttpProxyServer {


HttpProxyConnection::HttpProxyConnection(qintptr socketDescriptor, const QString &hostname, QObject *parent) : QObject(parent),
    socket_(nullptr), socketExternal_(nullptr), socketDescriptor_(socketDescriptor),
    hostname_(hostname), state_(READ_CLIENT_REQUEST), writeAllSocket_(nullptr),
    writeAllSocketExternal_(nullptr), httpError_(), bAlreadyClosedAndEmitFinished_(false)
{
    httpError_.status = HttpProxyReply::ok;
    //qDebug() << QThread::currentThreadId();
}

void HttpProxyConnection::forceClose()
{
    closeSocketsAndEmitFinished();
}

void HttpProxyConnection::start()
{
    //qDebug() << "start thread:" << QThread::currentThreadId();
    socket_ = new QTcpSocket(this);
    if (!socket_->setSocketDescriptor(socketDescriptor_))
    {
        return;
    }

    state_ = READ_CLIENT_REQUEST;
    connect(socket_, SIGNAL(disconnected()), SLOT(onSocketDisconnected()));
    connect(socket_, SIGNAL(readyRead()), SLOT(onSocketReadyRead()));
    writeAllSocket_ = new SocketWriteAll(this, socket_);
}

void HttpProxyConnection::onSocketDisconnected()
{
    //qCDebug(LOG_HTTP_SERVER) << "onSocketDisconnected connection closed.";
    closeSocketsAndEmitFinished();
}

void HttpProxyConnection::onSocketReadyRead()
{
    QByteArray arr = socket_->readAll();

    if (state_ == READ_CLIENT_REQUEST)
    {
        quint32 parsed;
        TRI_BOOL ret;
        ret = requestParser_.parse(arr, parsed);

        if (ret == TRI_TRUE)
        {
            if ((arr.size() - parsed) > 0)
            {
                extraContent_ = QByteArray(arr.data() + parsed, arr.size() - parsed);
            }

            if (requestParser_.getRequest().extractHostAndPort())
            {
                socketExternal_ = new QTcpSocket(this);

                connect(socketExternal_, &QTcpSocket::connected, this, &HttpProxyConnection::onExternalSocketConnected);
                connect(socketExternal_, &QTcpSocket::disconnected, this, &HttpProxyConnection::onExternalSocketDisconnected);
                connect(socketExternal_, &QTcpSocket::readyRead, this, &HttpProxyConnection::onExternalSocketReadyRead);
                connect(socketExternal_, &QTcpSocket::errorOccurred, this, &HttpProxyConnection::onExternalSocketError);

                writeAllSocketExternal_ = new SocketWriteAll(this, socketExternal_);

                state_ = CONNECTING_TO_EXTERNAL_SERVER;
                socketExternal_->connectToHost(QString::fromStdString(requestParser_.getRequest().host), requestParser_.getRequest().port);
            }
            else
            {
                //todo send error reply
                qCDebug(LOG_HTTP_SERVER) << "extractHostAndPort from request failed";
                closeSocketsAndEmitFinished();
            }
        }
        else if (ret == TRI_INDETERMINATE)
        {
        }
        else
        {
            httpError_ = HttpProxyReply::stock_reply(HttpProxyReply::service_unavailable);
            connect(writeAllSocket_, SIGNAL(allDataWriteFinished()), SLOT(onSocketAllDataWritten()));
            writeAllSocket_->write(httpError_.toBuffer());
            writeAllSocket_->setEmitAllDataWritten();
            state_ = STATE_WRITE_HTTP_ERROR;
            qCDebug(LOG_HTTP_SERVER) << "Parse client request failed";
        }
    }
    else if (state_ == CONNECTING_TO_EXTERNAL_SERVER)
    {
        extraContent_.append(arr);
    }
    else if (state_ == RELAY_BETWEEN_CLIENT_SERVER  || state_ == READ_HEADERS_FROM_WEBSERVER)
    {
        writeAllSocketExternal_->write(arr);
    }
    else
    {
        Q_ASSERT(false);
    }
}

void HttpProxyConnection::onSocketAllDataWritten()
{
    //qCDebug(LOG_HTTP_SERVER) << "onSocketAllDataWritten connection closed.";
    closeSocketsAndEmitFinished();
}

void HttpProxyConnection::onExternalSocketConnected()
{
    if (state_ == CONNECTING_TO_EXTERNAL_SERVER)
    {
        if (requestParser_.getRequest().isConnectMethod())
        {
            writeAllSocket_->write(QByteArray(reply_established_, strlen(reply_established_)));
            if (extraContent_.size() > 0)
            {
                writeAllSocketExternal_->write(extraContent_);
                extraContent_.clear();
            }
            state_ = RELAY_BETWEEN_CLIENT_SERVER;
        }
        else
        {
            std::string s = requestParser_.getRequest().getEstablishHttpConnectionMessage();
            writeAllSocketExternal_->write(QByteArray(s.c_str(), s.length()));

            s = requestParser_.getRequest().processClientHeaders();
            writeAllSocketExternal_->write(QByteArray(s.c_str(), s.length()));

            /*long contentLength = requestParser_.getRequest().getContentLength();
            if (contentLength > 0)
            {
                Q_ASSERT(contentLength == extraContent_.size());
                writeAllSocketExternal_->write(extraContent_);
            }
            else
            {*/
                if (extraContent_.size() > 0)
                {
                    writeAllSocketExternal_->write(extraContent_);
                    extraContent_.clear();
                }
            //}

            state_ = READ_HEADERS_FROM_WEBSERVER;
        }
    }
    else
    {
        Q_ASSERT(false);
    }
}

void HttpProxyConnection::onExternalSocketDisconnected()
{
    if (state_ != STATE_WRITE_HTTP_ERROR)
    {
        // wait while all data will be write to client socket
        if (writeAllSocket_)
        {
            connect(writeAllSocket_, SIGNAL(allDataWriteFinished()), SLOT(onSocketAllDataWritten()));
            writeAllSocket_->setEmitAllDataWritten();
        }
        else
        {
            closeSocketsAndEmitFinished();
        }
    }
}

void HttpProxyConnection::onExternalSocketReadyRead()
{
    QByteArray arr = socketExternal_->readAll();
    if (state_ == RELAY_BETWEEN_CLIENT_SERVER)
    {
        writeAllSocket_->write(arr);
    }
    else if (state_ == READ_HEADERS_FROM_WEBSERVER)
    {
        quint32 parsed;
        TRI_BOOL ret;
        ret = webAnswerParser_.parse(arr, parsed);

        if (ret == TRI_TRUE)
        {
            std::string s = webAnswerParser_.getAnswer().processServerHeaders(requestParser_.getRequest().http_version_major,
                                                                              requestParser_.getRequest().http_version_minor);
            writeAllSocket_->write(QByteArray(s.c_str(), s.length()));

            quint32 remainingData = arr.size() - parsed;
            if (remainingData > 0)
            {
                writeAllSocket_->write(QByteArray(arr.data() + parsed, remainingData));
            }
            state_ = RELAY_BETWEEN_CLIENT_SERVER;
        }
        else if (ret == TRI_FALSE)
        {
            //todo send error reply
            qCDebug(LOG_HTTP_SERVER) << "Parse webserver answer and headers failed";
            closeSocketsAndEmitFinished();
        }
    }
    else
    {
        Q_ASSERT(false);
    }
}

void HttpProxyConnection::onExternalSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    if (state_ == CONNECTING_TO_EXTERNAL_SERVER)
    {
        httpError_ = HttpProxyReply::stock_reply(HttpProxyReply::internal_server_error);
        connect(writeAllSocket_, SIGNAL(allDataWriteFinished()), SLOT(onSocketAllDataWritten()));
        writeAllSocket_->write(httpError_.toBuffer());
        writeAllSocket_->setEmitAllDataWritten();
        state_ = STATE_WRITE_HTTP_ERROR;
    }
}

void HttpProxyConnection::closeSocketsAndEmitFinished()
{
    if (!bAlreadyClosedAndEmitFinished_)
    {
        bAlreadyClosedAndEmitFinished_ = true;
        if (socket_)
        {
            socket_->close();
        }
        if (socketExternal_)
        {
            socketExternal_->close();
        }
        Q_EMIT finished(hostname_);
    }
}

} // namespace HttpProxyServer
