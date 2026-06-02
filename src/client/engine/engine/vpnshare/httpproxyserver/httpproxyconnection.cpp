#include "httpproxyconnection.h"
#include <QByteArray>
#include <QHostAddress>
#include <QThread>
#include "../proxydestinationfilter.h"
#include "utils/ws_assert.h"
#include "utils/log/categories.h"

namespace HttpProxyServer {

// Cap on bytes the client may send before we transition into RELAY. Bounds both the request parser's internal
// header buffers (defends against slowloris-style unauthenticated header drips) and any post-parse upload data
// queued while DNS + TCP connect to the upstream are in flight. 64 KiB is well above a TLS ClientHello (~500 B)
// and typical request-body chunks.
constexpr int kMaxPreRelayBytes = 64 * 1024;


HttpProxyConnection::HttpProxyConnection(qintptr socketDescriptor, const QString &hostname,
                                         const ProxyAuth::Config &auth, QObject *parent)
    : QObject(parent), socket_(nullptr), socketExternal_(nullptr), socketDescriptor_(socketDescriptor),
    hostname_(hostname), auth_(auth), state_(READ_CLIENT_REQUEST), writeAllSocket_(nullptr),
    writeAllSocketExternal_(nullptr), httpError_(), bAlreadyClosedAndEmitFinished_(false)
{
    httpError_.status = HttpProxyReply::ok;
}

void HttpProxyConnection::forceClose()
{
    closeSocketsAndEmitFinished();
}

void HttpProxyConnection::start()
{
    socket_ = new QTcpSocket(this);
    if (!socket_->setSocketDescriptor(socketDescriptor_)) {
        return;
    }

    state_ = READ_CLIENT_REQUEST;
    connect(socket_, &QTcpSocket::disconnected, this, &HttpProxyConnection::onSocketDisconnected);
    connect(socket_, &QTcpSocket::readyRead, this, &HttpProxyConnection::onSocketReadyRead);
    writeAllSocket_ = new SocketWriteAll(this, socket_);
}

void HttpProxyConnection::onSocketDisconnected()
{
    closeSocketsAndEmitFinished();
}

void HttpProxyConnection::onSocketReadyRead()
{
    QByteArray arr = socket_->readAll();

    if (state_ != RELAY_BETWEEN_CLIENT_SERVER && state_ != READ_HEADERS_FROM_WEBSERVER) {
        preRelayBytes_ += arr.size();
        if (preRelayBytes_ > kMaxPreRelayBytes) {
            qCWarning(LOG_HTTP_SERVER) << "Client exceeded pre-relay byte limit; closing";
            closeSocketsAndEmitFinished();
            return;
        }
    }

    if (state_ == READ_CLIENT_REQUEST) {
        quint32 parsed;
        TRI_BOOL ret;
        ret = requestParser_.parse(arr, parsed);

        if (ret == TRI_TRUE) {
            if ((arr.size() - parsed) > 0) {
                extraContent_ = QByteArray(arr.data() + parsed, arr.size() - parsed);
            }

            if (!sendChallenge()) {
                return;
            }

            if (requestParser_.getRequest().extractHostAndPort()) {
                state_ = RESOLVING_DESTINATION;
                dnsLookupCancelable_ = ProxyDestinationFilter::resolve(QString::fromStdString(requestParser_.getRequest().host), this,
                    [this](bool /*resolved*/, const QList<QHostAddress> &allowed) { onDestinationResolved(allowed); });
            } else {
                qCWarning(LOG_HTTP_SERVER) << "extractHostAndPort from request failed";
                writeError(HttpProxyReply::bad_request);
            }
        } else if (ret == TRI_INDETERMINATE) {
        } else {
            qCWarning(LOG_HTTP_SERVER) << "Parse client request failed";
            writeError(HttpProxyReply::service_unavailable);
        }
    } else if (state_ == RESOLVING_DESTINATION || state_ == CONNECTING_TO_EXTERNAL_SERVER) {
        extraContent_.append(arr);
    } else if (state_ == RELAY_BETWEEN_CLIENT_SERVER  || state_ == READ_HEADERS_FROM_WEBSERVER) {
        writeAllSocketExternal_->write(arr);
    } else {
        WS_ASSERT(false);
    }
}

bool HttpProxyConnection::sendChallenge()
{
    if (!auth_.required) {
        return true;
    }
    // RFC 7235: look for "Proxy-Authorization: Basic <base64(user:pw)>"
    const auto &headers = requestParser_.getRequest().headers;
    QByteArray providedToken;
    for (const auto &h : headers) {
        if (QString::fromStdString(h.name).compare(QStringLiteral("Proxy-Authorization"), Qt::CaseInsensitive) == 0) {
            QString value = QString::fromStdString(h.value).trimmed();
            if (value.startsWith(QStringLiteral("Basic "), Qt::CaseInsensitive)) {
                providedToken = QByteArray::fromBase64(value.mid(6).toUtf8());
            }
            break;
        }
    }
    QByteArray expected = (auth_.username + QStringLiteral(":") + auth_.password).toUtf8();
    if (!providedToken.isEmpty() &&
        !auth_.username.isEmpty() && !auth_.password.isEmpty() &&
        ProxyAuth::secureEqual(providedToken, expected)) {
        return true;
    }

    HttpProxyReply rep = HttpProxyReply::stock_reply(HttpProxyReply::proxy_authentication_required);
    HttpProxyHeader auth_header;
    auth_header.name = "Proxy-Authenticate";
    auth_header.value = "Basic realm=\"Windscribe Proxy Gateway\"";
    rep.headers.push_back(auth_header);
    httpError_ = rep;
    connect(writeAllSocket_, &SocketWriteAll::allDataWriteFinished, this, &HttpProxyConnection::onSocketAllDataWritten, Qt::UniqueConnection);
    writeAllSocket_->write(httpError_.toBuffer());
    writeAllSocket_->setEmitAllDataWritten();
    state_ = STATE_WRITE_HTTP_ERROR;
    return false;
}

void HttpProxyConnection::writeError(HttpProxyReply::status_type status)
{
    httpError_ = HttpProxyReply::stock_reply(status);
    connect(writeAllSocket_, &SocketWriteAll::allDataWriteFinished, this, &HttpProxyConnection::onSocketAllDataWritten, Qt::UniqueConnection);
    writeAllSocket_->write(httpError_.toBuffer());
    writeAllSocket_->setEmitAllDataWritten();
    state_ = STATE_WRITE_HTTP_ERROR;
}

void HttpProxyConnection::onDestinationResolved(const QList<QHostAddress> &allowedAddresses)
{
    if (state_ != RESOLVING_DESTINATION) {
        return;
    }
    if (allowedAddresses.isEmpty()) {
        writeError(HttpProxyReply::bad_gateway);
        return;
    }

    socketExternal_ = new QTcpSocket(this);
    connect(socketExternal_, &QTcpSocket::connected, this, &HttpProxyConnection::onExternalSocketConnected);
    connect(socketExternal_, &QTcpSocket::disconnected, this, &HttpProxyConnection::onExternalSocketDisconnected);
    connect(socketExternal_, &QTcpSocket::readyRead, this, &HttpProxyConnection::onExternalSocketReadyRead);
    connect(socketExternal_, &QTcpSocket::errorOccurred, this, &HttpProxyConnection::onExternalSocketError);

    writeAllSocketExternal_ = new SocketWriteAll(this, socketExternal_);

    state_ = CONNECTING_TO_EXTERNAL_SERVER;
    // Connect by validated IP, not the original hostname; protects against DNS rebinding swapping the resolved address
    // out from under us.
    socketExternal_->connectToHost(allowedAddresses.first(), requestParser_.getRequest().port);
}

void HttpProxyConnection::onSocketAllDataWritten()
{
    closeSocketsAndEmitFinished();
}

void HttpProxyConnection::onExternalSocketConnected()
{
    if (state_ == CONNECTING_TO_EXTERNAL_SERVER) {
        if (requestParser_.getRequest().isConnectMethod()) {
            writeAllSocket_->write(QByteArray(reply_established_, strlen(reply_established_)));
            if (extraContent_.size() > 0) {
                writeAllSocketExternal_->write(extraContent_);
                extraContent_.clear();
            }
            state_ = RELAY_BETWEEN_CLIENT_SERVER;
        } else {
            std::string s = requestParser_.getRequest().getEstablishHttpConnectionMessage();
            writeAllSocketExternal_->write(QByteArray(s.c_str(), s.length()));

            s = requestParser_.getRequest().processClientHeaders();
            writeAllSocketExternal_->write(QByteArray(s.c_str(), s.length()));

            /*long contentLength = requestParser_.getRequest().getContentLength();
            if (contentLength > 0)
            {
                WS_ASSERT(contentLength == extraContent_.size());
                writeAllSocketExternal_->write(extraContent_);
            }
            else
            {*/
                if (extraContent_.size() > 0) {
                    writeAllSocketExternal_->write(extraContent_);
                    extraContent_.clear();
                }
            //}

            state_ = READ_HEADERS_FROM_WEBSERVER;
        }
    } else {
        WS_ASSERT(false);
    }
}

void HttpProxyConnection::onExternalSocketDisconnected()
{
    if (state_ != STATE_WRITE_HTTP_ERROR) {
        // wait while all data will be write to client socket
        if (writeAllSocket_) {
            connect(writeAllSocket_, &SocketWriteAll::allDataWriteFinished, this, &HttpProxyConnection::onSocketAllDataWritten, Qt::UniqueConnection);
            writeAllSocket_->setEmitAllDataWritten();
        } else {
            closeSocketsAndEmitFinished();
        }
    }
}

void HttpProxyConnection::onExternalSocketReadyRead()
{
    QByteArray arr = socketExternal_->readAll();
    if (state_ == RELAY_BETWEEN_CLIENT_SERVER) {
        writeAllSocket_->write(arr);
    } else if (state_ == READ_HEADERS_FROM_WEBSERVER) {
        quint32 parsed;
        TRI_BOOL ret;
        ret = webAnswerParser_.parse(arr, parsed);

        if (ret == TRI_TRUE) {
            std::string s = webAnswerParser_.getAnswer().processServerHeaders(requestParser_.getRequest().http_version_major,
                                                                              requestParser_.getRequest().http_version_minor);
            writeAllSocket_->write(QByteArray(s.c_str(), s.length()));

            quint32 remainingData = arr.size() - parsed;
            if (remainingData > 0) {
                writeAllSocket_->write(QByteArray(arr.data() + parsed, remainingData));
            }
            state_ = RELAY_BETWEEN_CLIENT_SERVER;
        } else if (ret == TRI_FALSE) {
            //todo send error reply
            qCWarning(LOG_HTTP_SERVER) << "Parse webserver answer and headers failed";
            closeSocketsAndEmitFinished();
        }
    } else {
        WS_ASSERT(false);
    }
}

void HttpProxyConnection::onExternalSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    if (state_ == CONNECTING_TO_EXTERNAL_SERVER) {
        httpError_ = HttpProxyReply::stock_reply(HttpProxyReply::internal_server_error);
        connect(writeAllSocket_, &SocketWriteAll::allDataWriteFinished, this, &HttpProxyConnection::onSocketAllDataWritten, Qt::UniqueConnection);
        writeAllSocket_->write(httpError_.toBuffer());
        writeAllSocket_->setEmitAllDataWritten();
        state_ = STATE_WRITE_HTTP_ERROR;
    }
}

void HttpProxyConnection::closeSocketsAndEmitFinished()
{
    if (!bAlreadyClosedAndEmitFinished_) {
        // Cancel any in-flight DNS lookup before tearing down. cancel() shares a recursive_mutex with the wsnet
        // callback, so once it returns no worker thread can be inside the resolve() outer lambda — this is the
        // synchronization barrier that makes destruction race-free.
        if (dnsLookupCancelable_) {
            dnsLookupCancelable_->cancel();
            dnsLookupCancelable_.reset();
        }
        bAlreadyClosedAndEmitFinished_ = true;
        if (socket_) {
            socket_->close();
        }
        if (socketExternal_) {
            socketExternal_->close();
        }
        emit finished(hostname_);
    }
}

} // namespace HttpProxyServer
