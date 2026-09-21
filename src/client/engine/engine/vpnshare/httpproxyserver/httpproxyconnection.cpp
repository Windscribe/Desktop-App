#include "httpproxyconnection.h"
#include <QByteArray>
#include <QHostAddress>
#include <QThread>
#include <cstring>
#include "../proxydestinationfilter.h"
#include "../socketutils/nativesocket.h"
#include "httpproxyheader.h"  // for HttpProxyHeader (auth challenge)
#include "utils/log/categories.h"
#include "utils/ws_assert.h"

namespace HttpProxyServer {

// Cap on bytes the client may send before its request is complete. Bounds the request parser's header buffers against
// slowloris-style unauthenticated header drips; 64 KiB is well above any legitimate request line and headers.
constexpr int kMaxPreRelayBytes = 64 * 1024;

// Cap on response header bytes read from the upstream for plain HTTP requests. A hostile origin could otherwise
// stream header bytes without a terminating blank line and grow this process without bound.
constexpr int kMaxUpstreamHeaderBytes = 64 * 1024;

// Bounds what a relay holds in this process when one side stops reading: each socket buffers at most this much unread
// data, and a source is not read while its destination has this much unsent.
constexpr int kRelayBufferBytes = 64 * 1024;

// The client request and the resolve must each complete within this deadline. While the destination is connected there is
// no deadline (connecting itself is bounded by the socket's own connect timeout); once it closes, the drain to the client
// must keep making progress within this deadline.
int HttpProxyConnection::phaseTimeoutMs_ = 30000;

HttpProxyConnection::HttpProxyConnection(qintptr socketDescriptor, const QString &hostname,
                                         const ProxyAuth::Config &auth, QObject *parent)
    : QObject(parent), socket_(nullptr), socketExternal_(nullptr), socketDescriptor_(socketDescriptor),
    hostname_(hostname), auth_(auth), state_(READ_CLIENT_REQUEST), webAnswerParser_(kMaxUpstreamHeaderBytes),
    writeAllSocket_(nullptr),
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
        // The socket did not adopt the descriptor, so nothing else will ever release it.
        SocketUtils::closeNativeSocket(socketDescriptor_);
        closeSocketsAndEmitFinished();
        return;
    }

    phaseTimer_ = new QTimer(this);
    phaseTimer_->setSingleShot(true);
    connect(phaseTimer_, &QTimer::timeout, this, &HttpProxyConnection::closeSocketsAndEmitFinished);

    socket_->setReadBufferSize(kRelayBufferBytes);
    connect(socket_, &QTcpSocket::disconnected, this, &HttpProxyConnection::onSocketDisconnected);
    connect(socket_, &QTcpSocket::readyRead, this, &HttpProxyConnection::onSocketReadyRead);
    connect(socket_, &QTcpSocket::bytesWritten, this, &HttpProxyConnection::relayUpstreamToClient);
    writeAllSocket_ = new SocketWriteAll(this, socket_);
    setState(READ_CLIENT_REQUEST);
}

void HttpProxyConnection::onSocketDisconnected()
{
    // Our own close() lands here too, after it has already emptied the socket.
    if (bAlreadyClosedAndEmitFinished_) {
        return;
    }
    // Forward what the client sent before it left, congested upstream or not; the close below flushes it as far as it can.
    if (state_ == RELAY_BETWEEN_CLIENT_SERVER || state_ == READ_HEADERS_FROM_WEBSERVER) {
        const QByteArray arr = socket_->readAll();
        if (!arr.isEmpty()) {
            writeAllSocketExternal_->write(arr);
        }
    }
    closeSocketsAndEmitFinished();
}

void HttpProxyConnection::onSocketReadyRead()
{
    if (state_ == RELAY_BETWEEN_CLIENT_SERVER || state_ == READ_HEADERS_FROM_WEBSERVER) {
        relayClientToUpstream();
        return;
    }
    if (state_ == RELAY_DRAINING) {
        // The upstream is gone; keep taking the client's bytes so it is not blocked writing while it still owes us a read.
        socket_->readAll();
        return;
    }
    if (state_ == RESOLVING_DESTINATION || state_ == CONNECTING_TO_EXTERNAL_SERVER) {
        // Leave the bytes in the socket's bounded buffer; the kernel throttles the client until the upstream is up.
        return;
    }

    QByteArray arr = socket_->readAll();

    if (state_ == READ_CLIENT_REQUEST) {
        quint32 parsed;
        TRI_BOOL ret;
        ret = requestParser_.parse(arr, parsed);
        preRelayBytes_ += parsed;
        if (preRelayBytes_ > kMaxPreRelayBytes) {
            qCWarning(LOG_HTTP_SERVER) << "Client exceeded request byte limit; closing";
            closeSocketsAndEmitFinished();
            return;
        }

        if (ret == TRI_TRUE) {
            if ((arr.size() - parsed) > 0) {
                extraContent_ = QByteArray(arr.data() + parsed, arr.size() - parsed);
            }

            if (!sendChallenge()) {
                return;
            }

            if (requestParser_.getRequest().extractHostAndPort()) {
                setState(RESOLVING_DESTINATION);
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

    // Nothing the peer sends after an error reply is meaningful; stop reading so it cannot count against the request cap.
    disconnect(socket_, &QTcpSocket::readyRead, this, &HttpProxyConnection::onSocketReadyRead);
    HttpProxyReply rep = HttpProxyReply::stock_reply(HttpProxyReply::proxy_authentication_required);
    HttpProxyHeader auth_header;
    auth_header.name = "Proxy-Authenticate";
    auth_header.value = "Basic realm=\"Windscribe Proxy Gateway\"";
    rep.headers.push_back(auth_header);
    httpError_ = rep;
    connect(writeAllSocket_, &SocketWriteAll::allDataWriteFinished, this,
            &HttpProxyConnection::onSocketAllDataWritten, Qt::UniqueConnection);
    writeAllSocket_->write(httpError_.toBuffer());
    writeAllSocket_->setEmitAllDataWritten();
    setState(STATE_WRITE_HTTP_ERROR);
    return false;
}

void HttpProxyConnection::writeError(HttpProxyReply::status_type status)
{
    // Nothing the peer sends after an error reply is meaningful; stop reading so it cannot count against the request cap.
    disconnect(socket_, &QTcpSocket::readyRead, this, &HttpProxyConnection::onSocketReadyRead);
    httpError_ = HttpProxyReply::stock_reply(status);
    connect(writeAllSocket_, &SocketWriteAll::allDataWriteFinished, this,
            &HttpProxyConnection::onSocketAllDataWritten, Qt::UniqueConnection);
    writeAllSocket_->write(httpError_.toBuffer());
    writeAllSocket_->setEmitAllDataWritten();
    setState(STATE_WRITE_HTTP_ERROR);
}

void HttpProxyConnection::onDestinationResolved(const QList<QHostAddress> &allowedAddresses)
{
    if (bAlreadyClosedAndEmitFinished_ || state_ != RESOLVING_DESTINATION) {
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
    connect(socketExternal_, &QTcpSocket::bytesWritten, this, [this](qint64 bytes) {
        if (!bAlreadyClosedAndEmitFinished_ && state_ == READ_HEADERS_FROM_WEBSERVER && bytes > 0) {
            phaseTimer_->start(phaseTimeoutMs_);
        }
        relayClientToUpstream();
    });
    socketExternal_->setReadBufferSize(kRelayBufferBytes);

    writeAllSocketExternal_ = new SocketWriteAll(this, socketExternal_);

    setState(CONNECTING_TO_EXTERNAL_SERVER);
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
            setState(RELAY_BETWEEN_CLIENT_SERVER);
            relayClientToUpstream();
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

            setState(READ_HEADERS_FROM_WEBSERVER);
            relayClientToUpstream();
        }
    } else {
        WS_ASSERT(false);
    }
}

void HttpProxyConnection::onExternalSocketDisconnected()
{
    if (bAlreadyClosedAndEmitFinished_ || state_ == STATE_WRITE_HTTP_ERROR) {
        return;
    }
    if (state_ == READ_HEADERS_FROM_WEBSERVER) {
        writeError(HttpProxyReply::bad_gateway);
        return;
    }
    if (state_ != RELAY_BETWEEN_CLIENT_SERVER) {
        closeSocketsAndEmitFinished();
        return;
    }
    // Hand the client what the upstream already delivered; the pump closes once it is all written. Reading the client here
    // restarts its read notifier if congestion had paused it, so the discard path keeps the client unblocked meanwhile.
    setState(RELAY_DRAINING);
    socket_->readAll();
    relayUpstreamToClient();
}

void HttpProxyConnection::onExternalSocketReadyRead()
{
    if (state_ == RELAY_BETWEEN_CLIENT_SERVER) {
        relayUpstreamToClient();
    } else if (state_ == READ_HEADERS_FROM_WEBSERVER) {
        setState(READ_HEADERS_FROM_WEBSERVER);
        QByteArray arr = socketExternal_->readAll();
        quint32 parsed;
        const HttpProxyWebAnswerParser::Result ret = webAnswerParser_.parse(arr, parsed);

        if (ret == HttpProxyWebAnswerParser::Result::Complete) {
            std::string s = webAnswerParser_.getAnswer().processServerHeaders(requestParser_.getRequest().http_version_major,
                                                                              requestParser_.getRequest().http_version_minor);
            writeAllSocket_->write(QByteArray(s.c_str(), s.length()));

            quint32 remainingData = arr.size() - parsed;
            if (remainingData > 0) {
                writeAllSocket_->write(QByteArray(arr.data() + parsed, remainingData));
            }
            setState(RELAY_BETWEEN_CLIENT_SERVER);
        } else if (ret == HttpProxyWebAnswerParser::Result::TooLarge) {
            qCWarning(LOG_HTTP_SERVER) << "Response headers from" << QString::fromStdString(requestParser_.getRequest().host)
                                       << "exceeded" << kMaxUpstreamHeaderBytes << "bytes";
            writeError(HttpProxyReply::bad_gateway);
            socketExternal_->abort();
        } else if (ret == HttpProxyWebAnswerParser::Result::Malformed) {
            qCWarning(LOG_HTTP_SERVER) << "Parse webserver answer and headers failed";
            writeError(HttpProxyReply::bad_gateway);
            socketExternal_->abort();
        }
    } else {
        WS_ASSERT(false);
    }
}

void HttpProxyConnection::onExternalSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    if (state_ == CONNECTING_TO_EXTERNAL_SERVER) {
        writeError(HttpProxyReply::internal_server_error);
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
        if (phaseTimer_) {
            phaseTimer_->stop();
        }
        if (socket_) {
            socket_->close();
        }
        // A connected upstream still gets a best-effort flush of what the client sent. A connecting one is aborted,
        // because a plain close would let it report connected() later.
        if (socketExternal_) {
            if (socketExternal_->state() == QAbstractSocket::ConnectedState) {
                socketExternal_->close();
            } else {
                socketExternal_->abort();
            }
        }
        emit finished(hostname_);
    }
}

void HttpProxyConnection::setState(State newState)
{
    state_ = newState;
    // While the destination is connected there is no deadline (connecting is bounded by the socket's own connect timeout,
    // whose error path sends the reply) and an error reply completes on its own; every other phase gets a fresh deadline.
    if (newState == CONNECTING_TO_EXTERNAL_SERVER || newState == RELAY_BETWEEN_CLIENT_SERVER ||
        newState == STATE_WRITE_HTTP_ERROR) {
        phaseTimer_->stop();
    } else {
        phaseTimer_->start(phaseTimeoutMs_);
    }
}

void HttpProxyConnection::relayClientToUpstream()
{
    if (bAlreadyClosedAndEmitFinished_ || (state_ != RELAY_BETWEEN_CLIENT_SERVER && state_ != READ_HEADERS_FROM_WEBSERVER)) {
        return;
    }
    if (socketExternal_->bytesToWrite() < kRelayBufferBytes) {
        const QByteArray arr = socket_->readAll();
        if (!arr.isEmpty()) {
            writeAllSocketExternal_->write(arr);
        }
    }
}

void HttpProxyConnection::relayUpstreamToClient()
{
    if (bAlreadyClosedAndEmitFinished_ || (state_ != RELAY_BETWEEN_CLIENT_SERVER && state_ != RELAY_DRAINING)) {
        return;
    }
    if (socket_->bytesToWrite() < kRelayBufferBytes) {
        const QByteArray arr = socketExternal_->readAll();
        if (!arr.isEmpty()) {
            writeAllSocket_->write(arr);
        }
    }
    if (state_ == RELAY_DRAINING) {
        // Every call here is progress (the client took data, or the upstream just closed), so re-entering restarts the
        // deadline. A closed upstream leaves its unread bytes buffered; the relay is done once the client has them all.
        setState(RELAY_DRAINING);
        if (socketExternal_->bytesAvailable() == 0 && socket_->bytesToWrite() == 0) {
            closeSocketsAndEmitFinished();
        }
        return;
    }
}

} // namespace HttpProxyServer
