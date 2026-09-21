#include "socksproxyconnection.h"
#include <QByteArray>
#include <QHostAddress>
#include <QThread>
#include "../proxydestinationfilter.h"
#include "../socketutils/nativesocket.h"
#include "utils/log/categories.h"
#include "utils/ws_assert.h"

namespace SocksProxyServer {

// Bounds what a relay holds in this process when one side stops reading: each socket buffers at most this much unread
// data, and a source is not read while its destination has this much unsent.
constexpr int kRelayBufferBytes = 64 * 1024;

// The greeting, the authentication, the command and the resolve must each complete within this deadline. While the
// destination is connected there is no deadline (connecting itself is bounded by the socket's own connect timeout); once it
// closes, the drain to the client must keep making progress within this deadline.
int SocksProxyConnection::phaseTimeoutMs_ = 30000;

SocksProxyConnection::SocksProxyConnection(qintptr socketDescriptor, const QString &hostname,
                                           const ProxyAuth::Config &auth, QObject *parent)
    : QObject(parent), socket_(nullptr), socketExternal_(nullptr),
    socketDescriptor_(socketDescriptor), hostname_(hostname), auth_(auth), state_(READ_IDENT_REQ),
    writeAllSocket_(0), writeAllSocketExternal_(0), bAlreadyClosedAndEmitFinished_(false)
{
}

void SocksProxyConnection::start()
{
    //qCDebug(LOG_SOCKS_SERVER) << "start thread:" << QThread::currentThreadId();
    socket_ = new QTcpSocket(this);
    if (!socket_->setSocketDescriptor(socketDescriptor_)) {
        // The socket did not adopt the descriptor, so nothing else will ever release it.
        SocketUtils::closeNativeSocket(socketDescriptor_);
        closeSocketsAndEmitFinished();
        return;
    }
    phaseTimer_ = new QTimer(this);
    phaseTimer_->setSingleShot(true);
    connect(phaseTimer_, &QTimer::timeout, this, &SocksProxyConnection::closeSocketsAndEmitFinished);

    socket_->setReadBufferSize(kRelayBufferBytes);
    connect(socket_, &QTcpSocket::disconnected, this, &SocksProxyConnection::onSocketDisconnected);
    connect(socket_, &QTcpSocket::readyRead, this, &SocksProxyConnection::onSocketReadyRead);
    connect(socket_, &QTcpSocket::bytesWritten, this, &SocksProxyConnection::relayUpstreamToClient);
    writeAllSocket_ = new SocketWriteAll(this, socket_);
    setState(READ_IDENT_REQ);
}

void SocksProxyConnection::forceClose()
{
    closeSocketsAndEmitFinished();
}

void SocksProxyConnection::onSocketDisconnected()
{
    //qCDebug(LOG_SOCKS_SERVER) << "onSocketDisconnected connection closed.";
    // Our own close() lands here too, after it has already emptied the socket.
    if (bAlreadyClosedAndEmitFinished_) {
        return;
    }
    // Forward what the client sent before it left, congested upstream or not; the close below flushes it as far as it can.
    if (state_ == RELAY_BETWEEN_CLIENT_SERVER) {
        const QByteArray arr = socket_->readAll();
        if (!arr.isEmpty()) {
            writeAllSocketExternal_->write(arr);
        }
    }
    closeSocketsAndEmitFinished();
}

void SocksProxyConnection::onSocketReadyRead()
{
    if (state_ == RELAY_BETWEEN_CLIENT_SERVER) {
        relayClientToUpstream();
        return;
    }
    if (state_ == RELAY_DRAINING) {
        // The upstream is gone; keep taking the client's bytes so it is not blocked writing while it still owes us a read.
        socket_->readAll();
        return;
    }
    if (state_ == RESOLVING_DESTINATION || state_ == CONNECT_TO_HOST) {
        // Leave the bytes in the socket's bounded buffer; the kernel throttles the client until the upstream is up.
        return;
    }
    socketReadArr_.append(socket_->readAll());

    if (state_ == READ_IDENT_REQ) {
        handleIdentRequest();
    } else if (state_ == READ_AUTH) {
        handleAuthRequest();
    } else if (state_ == READ_COMMANDS) {
        handleCommandRequest();
    } else {
        qCCritical(LOG_SOCKS_SERVER) << "SocksProxyConnection::onSocketReadyRead() unknown state:" << state_;
        closeSocketsAndEmitFinished();
        WS_ASSERT(false);
    }
}

void SocksProxyConnection::handleIdentRequest()
{
    quint32 parsed;
    bool res = identReqParser_.parse(socketReadArr_, parsed);
    if (!res) {
        WS_ASSERT((quint32)socketReadArr_.size() == parsed);
        socketReadArr_.clear();
        return;
    }
    socketReadArr_.remove(0, parsed);

    if (identReqParser_.identReq().Version != 0x05) {
        socks5_answer answer;
        answer.Version = identReqParser_.identReq().Version;
        answer.Method = 0xFF;
        writeFinalReply(QByteArray((const char *)&answer, sizeof(answer)));
        return;
    }

    // Pick a method based on whether auth is required:
    //  - required ⇒ accept 0x02 (user/pw, RFC 1929)
    //  - optional ⇒ accept 0x00 (no-auth)
    const unsigned char wantedMethod = auth_.required ? 0x02 : 0x00;
    bool offered = false;
    for (unsigned char i = 0; i < identReqParser_.identReq().NumberOfMethods; ++i) {
        if (identReqParser_.identReq().Methods[i] == wantedMethod) {
            offered = true;
            break;
        }
    }

    socks5_answer answer;
    answer.Version = 0x05;
    if (offered) {
        answer.Method = wantedMethod;
        setState(auth_.required ? READ_AUTH : READ_COMMANDS);
        writeAllSocket_->write(QByteArray((const char *)&answer, sizeof(answer)));
        if (!socketReadArr_.isEmpty()) {
            // Pipelined bytes after the method selection.
            if (state_ == READ_AUTH) {
                handleAuthRequest();
            } else {
                handleCommandRequest();
            }
        }
    } else {
        answer.Method = 0xFF;
        writeFinalReply(QByteArray((const char *)&answer, sizeof(answer)));
    }
}

void SocksProxyConnection::handleAuthRequest()
{
    // RFC 1929 sub-negotiation:
    //   +----+------+----------+------+----------+
    //   |VER | ULEN |  UNAME   | PLEN |  PASSWD  |
    //   +----+------+----------+------+----------+
    //   | 1  |  1   | 1 to 255 |  1   | 1 to 255 |
    //   +----+------+----------+------+----------+
    // VER for sub-negotiation is 0x01, not 0x05.
    if (socketReadArr_.size() < 2) return;
    const quint8 ver = static_cast<quint8>(socketReadArr_.at(0));
    const quint8 ulen = static_cast<quint8>(socketReadArr_.at(1));
    if (socketReadArr_.size() < 2 + ulen + 1) return;
    const quint8 plen = static_cast<quint8>(socketReadArr_.at(2 + ulen));
    const int total = 2 + ulen + 1 + plen;
    if (socketReadArr_.size() < total) return;

    QByteArray providedUser = socketReadArr_.mid(2, ulen);
    QByteArray providedPass = socketReadArr_.mid(2 + ulen + 1, plen);
    socketReadArr_.remove(0, total);

    // Run both compares unconditionally so timing doesn't leak whether the username matched.
    const bool userOk = ProxyAuth::secureEqual(providedUser, auth_.username.toUtf8());
    const bool passOk = ProxyAuth::secureEqual(providedPass, auth_.password.toUtf8());
    const bool ok = (ver == 0x01) && !auth_.username.isEmpty() && !auth_.password.isEmpty() && userOk && passOk;

    char reply[2] = { 0x01, static_cast<char>(ok ? 0x00 : 0x01) };
    if (!ok) {
        writeFinalReply(QByteArray(reply, sizeof(reply)));
        return;
    }
    writeAllSocket_->write(QByteArray(reply, sizeof(reply)));
    setState(READ_COMMANDS);
    if (!socketReadArr_.isEmpty()) {
        // Pipelined command bytes after auth.
        handleCommandRequest();
    }
}

void SocksProxyConnection::handleCommandRequest()
{
    quint32 parsed;
    TRI_BOOL res = commandParser_.parse(socketReadArr_, parsed);
    if (res == TRI_INDETERMINATE) {
        socketReadArr_.remove(0, parsed);
        return;
    }
    if (res != TRI_TRUE) {
        qCCritical(LOG_SOCKS_SERVER) << "SocksProxyConnection: incorrect input command packet";
        const quint8 addrType = commandParser_.cmd().AddrType;
        const bool knownAddrType = addrType == 0x01 || addrType == 0x03 || addrType == 0x04;
        sendReply(knownAddrType ? 0x01 : 0x08);  // general failure, or address type not supported
        return;
    }
    socketReadArr_.remove(0, parsed);

    if (commandParser_.cmd().Cmd != 0x01) {
        // 0x07 == "command not supported"
        qCWarning(LOG_SOCKS_SERVER) << "SocksProxyConnection: unsupported command" << commandParser_.cmd().Cmd;
        sendReply(0x07);
        return;
    }

    WS_ASSERT(socketExternal_ == nullptr);

    if (commandParser_.cmd().AddrType == 0x01) {  // ip4
        quint32 ipv4;
        memcpy(&ipv4, &commandParser_.cmd().DestAddr.IPv4, sizeof(quint32));
        QHostAddress addr(ntohl(ipv4));
        if (!ProxyDestinationFilter::isAllowedDestination(addr)) {
            sendReply(0x02);  // not allowed by ruleset
            return;
        }
        connectExternal(addr, ntohs(commandParser_.cmd().DestPort));
    } else if (commandParser_.cmd().AddrType == 0x04) {  // ip6
        quint8 *ip6Addr = (quint8 *)&commandParser_.cmd().DestAddr.IPv6;
        QHostAddress addr(ip6Addr);
        if (!ProxyDestinationFilter::isAllowedDestination(addr)) {
            sendReply(0x02);
            return;
        }
        connectExternal(addr, ntohs(commandParser_.cmd().DestPort));
    } else if (commandParser_.cmd().AddrType == 0x03) {  // domain
        std::string hostname(commandParser_.cmd().DestAddr.Domain, commandParser_.cmd().DestAddr.DomainLen);
        const quint16 port = ntohs(commandParser_.cmd().DestPort);
        setState(RESOLVING_DESTINATION);
        dnsLookupCancelable_ = ProxyDestinationFilter::resolve(QString::fromStdString(hostname), this,
            [this, port, hostname](bool resolved, const QList<QHostAddress> &allowed) {
                if (bAlreadyClosedAndEmitFinished_ || state_ != RESOLVING_DESTINATION) {
                    return;
                }
                if (!resolved) {
                    qCWarning(LOG_SOCKS_SERVER) << "DNS lookup failed for SOCKS5 destination" << QString::fromStdString(hostname);
                    sendReply(0x04);  // host unreachable
                    return;
                }
                if (allowed.isEmpty()) {
                    sendReply(0x02);  // not allowed by ruleset
                    return;
                }
                connectExternal(allowed.first(), port);
            });
    }
}

void SocksProxyConnection::connectExternal(const QHostAddress &addr, quint16 port)
{
    socketExternal_ = new QTcpSocket(this);
    connect(socketExternal_, &QTcpSocket::connected, this, &SocksProxyConnection::onExternalSocketConnected);
    connect(socketExternal_, &QTcpSocket::disconnected, this, &SocksProxyConnection::onExternalSocketDisconnected);
    connect(socketExternal_, &QTcpSocket::readyRead, this, &SocksProxyConnection::onExternalSocketReadyRead);
    connect(socketExternal_, &QTcpSocket::errorOccurred, this, &SocksProxyConnection::onExternalSocketError);
    connect(socketExternal_, &QTcpSocket::bytesWritten, this, &SocksProxyConnection::relayClientToUpstream);
    socketExternal_->setReadBufferSize(kRelayBufferBytes);

    writeAllSocketExternal_ = new SocketWriteAll(this, socketExternal_);
    setState(CONNECT_TO_HOST);
    socketExternal_->connectToHost(addr, port);
}

void SocksProxyConnection::sendReply(quint8 reply)
{
    socks5_resp resp;
    memset(&resp, 0, sizeof(resp));
    resp.Version = 0x05;
    resp.Reply = reply;
    resp.Reserved = 0;
    resp.AddrType = 0x01;
    resp.BindPort = 0;
    writeFinalReply(getByteArrayFromSocks5Resp(resp));
}

void SocksProxyConnection::writeFinalReply(const QByteArray &reply)
{
    // Nothing the peer sends after a final reply is meaningful; stop reading so it cannot re-enter the parsers.
    // The reply is small enough to complete into the kernel buffer whether or not the peer reads, so no deadline.
    disconnect(socket_, &QTcpSocket::readyRead, this, &SocksProxyConnection::onSocketReadyRead);
    state_ = WRITE_FINAL_REPLY;
    phaseTimer_->stop();
    writeAllSocket_->write(reply);
    connect(writeAllSocket_, &SocketWriteAll::allDataWriteFinished, this, &SocksProxyConnection::closeSocketsAndEmitFinished,
            Qt::UniqueConnection);
    writeAllSocket_->setEmitAllDataWritten();
}

void SocksProxyConnection::onExternalSocketConnected()
{
    if (state_ == CONNECT_TO_HOST) {
        socks5_resp resp;
        memcpy(&resp, &commandParser_.cmd(), sizeof(resp));
        resp.Reply = 0x00;
        //resp.BindPort = 0x00;
        //memset(&resp.BindAddr.IPv4, 0, sizeof(resp.BindAddr.IPv4));
        writeAllSocket_->write(getByteArrayFromSocks5Resp(resp));
        setState(RELAY_BETWEEN_CLIENT_SERVER);
        // Flush any client bytes pipelined after the command, then whatever the socket buffered while we connected.
        if (!socketReadArr_.isEmpty()) {
            writeAllSocketExternal_->write(socketReadArr_);
            socketReadArr_.clear();
        }
        relayClientToUpstream();
    } else {
        WS_ASSERT(false);
    }
}

void SocksProxyConnection::onExternalSocketDisconnected()
{
    if (bAlreadyClosedAndEmitFinished_ || state_ == WRITE_FINAL_REPLY) {
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

void SocksProxyConnection::onExternalSocketReadyRead()
{
    relayUpstreamToClient();
}

void SocksProxyConnection::onExternalSocketError(QAbstractSocket::SocketError socketError)
{
    if (state_ == CONNECT_TO_HOST) {
        // Translate the Qt error into a SOCKS5 reply byte (RFC 1928 §6).
        quint8 reply;
        switch (socketError) {
            case QAbstractSocket::ConnectionRefusedError:
                reply = 0x05;  // connection refused
                break;
            case QAbstractSocket::HostNotFoundError:
                reply = 0x04;  // host unreachable
                break;
            case QAbstractSocket::SocketTimeoutError:
                reply = 0x06;  // TTL expired
                break;
            case QAbstractSocket::NetworkError:
                reply = 0x03;  // network unreachable
                break;
            default:
                reply = 0x01;  // general SOCKS server failure
                break;
        }
        qCWarning(LOG_SOCKS_SERVER) << "External connect failed:" << socketExternal_->errorString() << "reply" << reply;
        sendReply(reply);
    }
    // Any other error is followed by disconnected(), which hands the client whatever the upstream already delivered.
}

void SocksProxyConnection::closeSocketsAndEmitFinished()
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

void SocksProxyConnection::setState(State newState)
{
    state_ = newState;
    // Relay has no deadline and connecting is bounded by the socket's own connect timeout (whose error path sends the
    // reply); every other phase gets a fresh deadline.
    if (newState == RELAY_BETWEEN_CLIENT_SERVER || newState == CONNECT_TO_HOST) {
        phaseTimer_->stop();
    } else {
        phaseTimer_->start(phaseTimeoutMs_);
    }
}

void SocksProxyConnection::relayClientToUpstream()
{
    if (bAlreadyClosedAndEmitFinished_ || (state_ != RELAY_BETWEEN_CLIENT_SERVER)) {
        return;
    }
    if (socketExternal_->bytesToWrite() < kRelayBufferBytes) {
        const QByteArray arr = socket_->readAll();
        if (!arr.isEmpty()) {
            writeAllSocketExternal_->write(arr);
        }
    }
}

void SocksProxyConnection::relayUpstreamToClient()
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

QByteArray SocksProxyConnection::getByteArrayFromSocks5Resp(const socks5_resp &resp)
{
    QByteArray arr;
    int size = sizeof(resp.Version) + sizeof(resp.Reply) + sizeof(resp.Reserved) + sizeof(resp.AddrType);
    if (resp.AddrType == 0x01) {  // ip4
        size += sizeof(resp.BindAddr.IPv4);
    } else if (resp.AddrType == 0x04) {  // ip6
        size += sizeof(resp.BindAddr.IPv6);
    } else if (resp.AddrType == 0x03) {  // domain name
        size += sizeof(resp.BindAddr.DomainLen) + resp.BindAddr.DomainLen;
    }
    size += sizeof(resp.BindPort);
    arr.resize(size);

    char *p = arr.data();
    int len_first_4 = sizeof(resp.Version) + sizeof(resp.Reply) + sizeof(resp.Reserved) + sizeof(resp.AddrType);
    memcpy(p, &resp, len_first_4);

    if (resp.AddrType == 0x01) {  // ip4
        memcpy(p + len_first_4, &resp.BindAddr.IPv4, sizeof(resp.BindAddr.IPv4));
        memcpy(p + len_first_4 + sizeof(resp.BindAddr.IPv4), &resp.BindPort, sizeof(resp.BindPort));
    } else if (resp.AddrType == 0x04) {  // ip6
        memcpy(p + len_first_4, &resp.BindAddr.IPv6, sizeof(resp.BindAddr.IPv6));
        memcpy(p + len_first_4 + sizeof(resp.BindAddr.IPv6), &resp.BindPort, sizeof(resp.BindPort));
    } else if (resp.AddrType == 0x03) {  // domain name
        memcpy(p + len_first_4, &resp.BindAddr.DomainLen, sizeof(resp.BindAddr.DomainLen));
        memcpy(p + len_first_4 + sizeof(resp.BindAddr.DomainLen), &resp.BindAddr.Domain[0], resp.BindAddr.DomainLen);
        memcpy(p + len_first_4 + sizeof(resp.BindAddr.DomainLen) + resp.BindAddr.DomainLen, &resp.BindPort, sizeof(resp.BindPort));
    }

    return arr;
}


} // namespace SocksProxyServer
