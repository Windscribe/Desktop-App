#include "socksproxyconnection.h"
#include <QByteArray>
#include <QHostAddress>
#include <QThread>
#include "../proxydestinationfilter.h"
#include "utils/ws_assert.h"
#include "utils/log/categories.h"

namespace SocksProxyServer {

// Cap on client bytes pipelined after the SOCKS5 command and before we transition into relay. Without this, an
// upload to an unreachable/slow upstream could balloon the per-connection buffer for tens of seconds (DNS + TCP
// connect timeouts). 64 KiB is well above any expected pre-relay payload.
constexpr int kMaxBufferedBytesBeforeRelay = 64 * 1024;


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
        return;
    }
    state_ = READ_IDENT_REQ;
    readExactly_.reset(new SocksProxyReadExactly(sizeof(socks5_ident_req)));
    connect(socket_, &QTcpSocket::disconnected, this, &SocksProxyConnection::onSocketDisconnected);
    connect(socket_, &QTcpSocket::readyRead, this, &SocksProxyConnection::onSocketReadyRead);
    writeAllSocket_ = new SocketWriteAll(this, socket_);
}

void SocksProxyConnection::forceClose()
{
    closeSocketsAndEmitFinished();
}

void SocksProxyConnection::onSocketDisconnected()
{
    //qCDebug(LOG_SOCKS_SERVER) << "onSocketDisconnected connection closed.";
    closeSocketsAndEmitFinished();
}

void SocksProxyConnection::onSocketReadyRead()
{
    socketReadArr_.append(socket_->readAll());

    if (state_ == READ_IDENT_REQ) {
        handleIdentRequest();
    } else if (state_ == READ_AUTH) {
        handleAuthRequest();
    } else if (state_ == READ_COMMANDS) {
        handleCommandRequest();
    } else if (state_ == RESOLVING_DESTINATION || state_ == CONNECT_TO_HOST) {
        // Buffer client bytes that arrive while we're resolving / connecting. They'll be flushed in
        // onExternalSocketConnected once we transition to relay.
        if (socketReadArr_.size() > kMaxBufferedBytesBeforeRelay) {
            qCWarning(LOG_SOCKS_SERVER) << "Client buffered" << socketReadArr_.size()
                                        << "bytes during resolve/connect; closing";
            closeSocketsAndEmitFinished();
            return;
        }
    } else if (state_ == RELAY_BETWEEN_CLIENT_SERVER) {
        writeAllSocketExternal_->write(socketReadArr_);
        socketReadArr_.clear();
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
        writeAllSocket_->write(QByteArray((const char *)&answer, sizeof(answer)));
        connect(writeAllSocket_, &SocketWriteAll::allDataWriteFinished, this, &SocksProxyConnection::closeSocketsAndEmitFinished, Qt::UniqueConnection);
        writeAllSocket_->setEmitAllDataWritten();
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
        state_ = auth_.required ? READ_AUTH : READ_COMMANDS;
        writeAllSocket_->write(QByteArray((const char *)&answer, sizeof(answer)));
    } else {
        answer.Method = 0xFF;
        writeAllSocket_->write(QByteArray((const char *)&answer, sizeof(answer)));
        connect(writeAllSocket_, &SocketWriteAll::allDataWriteFinished, this, &SocksProxyConnection::closeSocketsAndEmitFinished, Qt::UniqueConnection);
        writeAllSocket_->setEmitAllDataWritten();
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
    writeAllSocket_->write(QByteArray(reply, sizeof(reply)));
    if (!ok) {
        connect(writeAllSocket_, &SocketWriteAll::allDataWriteFinished, this, &SocksProxyConnection::closeSocketsAndEmitFinished, Qt::UniqueConnection);
        writeAllSocket_->setEmitAllDataWritten();
        return;
    }
    state_ = READ_COMMANDS;
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
        closeSocketsAndEmitFinished();
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
        QHostAddress addr(ipv4);
        if (!ProxyDestinationFilter::isAllowedDestination(addr)) {
            sendReply(0x02);  // not allowed by ruleset
            return;
        }
        connectExternal(addr, commandParser_.cmd().DestPort);
    } else if (commandParser_.cmd().AddrType == 0x04) {  // ip6
        quint8 *ip6Addr = (quint8 *)&commandParser_.cmd().DestAddr.IPv6;
        QHostAddress addr(ip6Addr);
        if (!ProxyDestinationFilter::isAllowedDestination(addr)) {
            sendReply(0x02);
            return;
        }
        connectExternal(addr, commandParser_.cmd().DestPort);
    } else if (commandParser_.cmd().AddrType == 0x03) {  // domain
        std::string hostname(commandParser_.cmd().DestAddr.Domain, commandParser_.cmd().DestAddr.DomainLen);
        const quint16 port = commandParser_.cmd().DestPort;
        state_ = RESOLVING_DESTINATION;
        dnsLookupCancelable_ = ProxyDestinationFilter::resolve(QString::fromStdString(hostname), this,
            [this, port, hostname](bool resolved, const QList<QHostAddress> &allowed) {
                if (state_ != RESOLVING_DESTINATION) return;
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
    } else {
        qCWarning(LOG_SOCKS_SERVER) << "SocksProxyConnection: unsupported addr type" << commandParser_.cmd().AddrType;
        sendReply(0x08);  // address type not supported
    }
}

void SocksProxyConnection::connectExternal(const QHostAddress &addr, quint16 port)
{
    socketExternal_ = new QTcpSocket(this);
    connect(socketExternal_, &QTcpSocket::connected, this, &SocksProxyConnection::onExternalSocketConnected);
    connect(socketExternal_, &QTcpSocket::disconnected, this, &SocksProxyConnection::onExternalSocketDisconnected);
    connect(socketExternal_, &QTcpSocket::readyRead, this, &SocksProxyConnection::onExternalSocketReadyRead);
    connect(socketExternal_, &QTcpSocket::errorOccurred, this, &SocksProxyConnection::onExternalSocketError);

    writeAllSocketExternal_ = new SocketWriteAll(this, socketExternal_);
    state_ = CONNECT_TO_HOST;
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
    writeAllSocket_->write(getByteArrayFromSocks5Resp(resp));
    connect(writeAllSocket_, &SocketWriteAll::allDataWriteFinished, this, &SocksProxyConnection::closeSocketsAndEmitFinished, Qt::UniqueConnection);
    writeAllSocket_->setEmitAllDataWritten();
}

/*void HttpProxyConnection::onSocketAllDataWritten()
{
    qCDebug(LOG_HTTP_SERVER) << "onSocketAllDataWritten connection closed.";
    closeSocketsAndEmitFinished();
}*/

void SocksProxyConnection::onExternalSocketConnected()
{
    if (state_ == CONNECT_TO_HOST) {
        socks5_resp resp;
        memcpy(&resp, &commandParser_.cmd(), sizeof(resp));
        resp.Reply = 0x00;
        //resp.BindPort = 0x00;
        //memset(&resp.BindAddr.IPv4, 0, sizeof(resp.BindAddr.IPv4));
        writeAllSocket_->write(getByteArrayFromSocks5Resp(resp));
        state_ = RELAY_BETWEEN_CLIENT_SERVER;
        // Flush any client bytes that arrived during resolution/connect.
        if (!socketReadArr_.isEmpty()) {
            writeAllSocketExternal_->write(socketReadArr_);
            socketReadArr_.clear();
        }
    } else {
        WS_ASSERT(false);
    }
}

void SocksProxyConnection::onExternalSocketDisconnected()
{
    // Drain any pending writes to the client and then close. If we don't have
    // a writer (shouldn't normally happen post-relay), close immediately.
    if (writeAllSocket_) {
        connect(writeAllSocket_, &SocketWriteAll::allDataWriteFinished, this, &SocksProxyConnection::closeSocketsAndEmitFinished, Qt::UniqueConnection);
        writeAllSocket_->setEmitAllDataWritten();
    } else {
        closeSocketsAndEmitFinished();
    }
}

void SocksProxyConnection::onExternalSocketReadyRead()
{
    QByteArray arr = socketExternal_->readAll();
    if (state_ == RELAY_BETWEEN_CLIENT_SERVER) {
        writeAllSocket_->write(arr);
    }
    /*else if (state_ == READ_HEADERS_FROM_WEBSERVER)
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
        WS_ASSERT(false);
    }*/
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
    } else {
        // Mid-relay error — just close.
        closeSocketsAndEmitFinished();
    }
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
        if (socket_) {
            socket_->close();
        }
        if (socketExternal_) {
            socketExternal_->close();
        }
        emit finished(hostname_);
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
