#include "socksproxyconnection.h"
#include <QHostAddress>
#include <QThread>
#include "utils/ws_assert.h"
#include "utils/logger.h"

namespace SocksProxyServer {


SocksProxyConnection::SocksProxyConnection(qintptr socketDescriptor, const QString &hostname,
                                           QObject *parent)
    : QObject(parent), socket_(nullptr), socketExternal_(nullptr),
    socketDescriptor_(socketDescriptor), hostname_(hostname), state_(READ_IDENT_REQ),
    writeAllSocket_(0), writeAllSocketExternal_(0), bAlreadyClosedAndEmitFinished_(false)
{
}

void SocksProxyConnection::start()
{
    //qCDebug(LOG_SOCKS_SERVER) << "start thread:" << QThread::currentThreadId();
    socket_ = new QTcpSocket(this);
    if (!socket_->setSocketDescriptor(socketDescriptor_))
    {
        return;
    }
    state_ = READ_IDENT_REQ;
    readExactly_.reset(new SocksProxyReadExactly(sizeof(socks5_ident_req)));
    connect(socket_, SIGNAL(disconnected()), SLOT(onSocketDisconnected()));
    connect(socket_, SIGNAL(readyRead()), SLOT(onSocketReadyRead()));
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

    if (state_ == READ_IDENT_REQ)
    {
        quint32 parsed;
        bool res = identReqParser_.parse(socketReadArr_, parsed);

        if (res)
        {
            socketReadArr_.remove(0, parsed);

            bool bFindedNonAuthMethod = false;
            for (unsigned char i = 0; i < identReqParser_.identReq().NumberOfMethods; ++i)
            {
                if (identReqParser_.identReq().Methods[i] == 0x00)
                {
                    bFindedNonAuthMethod = true;
                    break;
                }
            }

            socks5_answer answer;
            if (bFindedNonAuthMethod && identReqParser_.identReq().Version == 0x05)
            {
                answer.Version = identReqParser_.identReq().Version;
                answer.Method = 0x00;
                state_ = READ_COMMANDS;
                writeAllSocket_->write(QByteArray((const char *)&answer, sizeof(answer)));
            }
            else
            {
                answer.Version = identReqParser_.identReq().Version;
                answer.Method = 0xFF;
                writeAllSocket_->write(QByteArray((const char *)&answer, sizeof(answer)));
                connect(writeAllSocket_, SIGNAL(allDataWriteFinished()), SLOT(closeSocketsAndEmitFinished()));
                writeAllSocket_->setEmitAllDataWritten();
            }
        }
        else
        {
            WS_ASSERT((quint32)socketReadArr_.size() == parsed);
            socketReadArr_.clear();
        }
    }
    else if (state_ == READ_COMMANDS)
    {
        quint32 parsed;
        TRI_BOOL res = commandParser_.parse(socketReadArr_, parsed);
        if (res == TRI_TRUE)
        {
            /*quint32 ipv4;
            memcpy(&ipv4, &commandParser_.cmd().DestAddr.IPv4, sizeof(quint32));
            QHostAddress ha(ipv4);
            qDebug() << ha.toString();*/
            socketReadArr_.remove(0, parsed);

            // handle command
            if (commandParser_.cmd().Cmd == 0x01)  // connect
            {
                WS_ASSERT(socketExternal_ == NULL);
                socketExternal_ = new QTcpSocket(this);

                connect(socketExternal_, &QTcpSocket::connected, this, &SocksProxyConnection::onExternalSocketConnected);
                connect(socketExternal_, &QTcpSocket::disconnected, this, &SocksProxyConnection::onExternalSocketDisconnected);
                connect(socketExternal_, &QTcpSocket::readyRead, this, &SocksProxyConnection::onExternalSocketReadyRead);
                connect(socketExternal_, &QTcpSocket::errorOccurred, this, &SocksProxyConnection::onExternalSocketError);

                writeAllSocketExternal_ = new SocketWriteAll(this, socketExternal_);
                state_ = CONNECT_TO_HOST;

                if (commandParser_.cmd().AddrType == 0x01)  // ip4
                {
                    quint32 ipv4;
                    WS_ASSERT(sizeof(commandParser_.cmd().DestAddr.IPv4) == sizeof(ipv4));
                    memcpy(&ipv4, &commandParser_.cmd().DestAddr.IPv4, sizeof(quint32));
                    socketExternal_->connectToHost(QHostAddress(ipv4), commandParser_.cmd().DestPort);
                }
                else if (commandParser_.cmd().AddrType == 0x04)  // ip6
                {
                    quint8 *ip6Addr = (quint8 *)&commandParser_.cmd().DestAddr.IPv6;
                    socketExternal_->connectToHost(QHostAddress(ip6Addr), commandParser_.cmd().DestPort);
                }
                else if (commandParser_.cmd().AddrType == 0x03)  // domain name
                {
                    std::string hostname(commandParser_.cmd().DestAddr.Domain, commandParser_.cmd().DestAddr.DomainLen);
                    socketExternal_->connectToHost(QString::fromStdString(hostname), commandParser_.cmd().DestPort);
                }
            }
            else if (commandParser_.cmd().Cmd == 0x02)  // bind
            {
                WS_ASSERT(false);
            }
            else if (commandParser_.cmd().Cmd == 0x03)  // udp associate
            {
                WS_ASSERT(false);
            }
            else
            {
                qCDebug(LOG_SOCKS_SERVER) << "SocksProxyConnection::onSocketReadyRead() unknown command:" << commandParser_.cmd().Cmd;
                closeSocketsAndEmitFinished();
                WS_ASSERT(false);
            }
        }
        else if (res == TRI_INDETERMINATE)
        {
            socketReadArr_.remove(0, parsed);
        }
        else
        {
            qCDebug(LOG_SOCKS_SERVER) << "SocksProxyConnection::onSocketReadyRead() incorrect input command packet";
            closeSocketsAndEmitFinished();
            WS_ASSERT(false);
        }
    }
    else if (state_ == RELAY_BETWEEN_CLIENT_SERVER)
    {
        writeAllSocketExternal_->write(socketReadArr_);
        socketReadArr_.clear();
    }
    else
    {
        qCDebug(LOG_SOCKS_SERVER) << "SocksProxyConnection::onSocketReadyRead() unknown state:" << state_;
        closeSocketsAndEmitFinished();
        WS_ASSERT(false);
    }
}

/*void HttpProxyConnection::onSocketAllDataWritten()
{
    qCDebug(LOG_HTTP_SERVER) << "onSocketAllDataWritten connection closed.";
    closeSocketsAndEmitFinished();
}*/

void SocksProxyConnection::onExternalSocketConnected()
{
    if (state_ == CONNECT_TO_HOST)
    {
        socks5_resp resp;
        memcpy(&resp, &commandParser_.cmd(), sizeof(resp));
        resp.Reply = 0x00;
        //resp.BindPort = 0x00;
        //memset(&resp.BindAddr.IPv4, 0, sizeof(resp.BindAddr.IPv4));
        writeAllSocket_->write(getByteArrayFromSocks5Resp(resp));
        state_ = RELAY_BETWEEN_CLIENT_SERVER;
    }
    else
    {
        WS_ASSERT(false);
    }
}

void SocksProxyConnection::onExternalSocketDisconnected()
{
    /*// wait while all data will be write to client socket
    if (writeAllSocket_)
    {
        connect(writeAllSocket_, SIGNAL(allDataWriteFinished()), SLOT(onSocketAllDataWritten()));
        writeAllSocket_->setEmitAllDataWritten();
    }
    else
    {
        closeSocketsAndEmitFinished();
    }*/
}

void SocksProxyConnection::onExternalSocketReadyRead()
{
    QByteArray arr = socketExternal_->readAll();
    if (state_ == RELAY_BETWEEN_CLIENT_SERVER)
    {
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
    Q_UNUSED(socketError);
    if (state_ == CONNECT_TO_HOST)
    {
    }
    /*Q_UNUSED(socketError);
    if (state_ == CONNECTING_TO_EXTERNAL_SERVER)
    {
        //todo send error reply
        qCDebug(LOG_HTTP_SERVER) << "Failed connect to external host:" << QString::fromStdString(requestParser_.getRequest().host) << ":"
                                 << requestParser_.getRequest().port;
        closeSocketsAndEmitFinished();
    }*/
}

void SocksProxyConnection::closeSocketsAndEmitFinished()
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
        emit finished(hostname_);
    }
}

QByteArray SocksProxyConnection::getByteArrayFromSocks5Resp(const socks5_resp &resp)
{
    QByteArray arr;
    int size = sizeof(resp.Version) + sizeof(resp.Reply) + sizeof(resp.Reserved) + sizeof(resp.AddrType);
    if (resp.AddrType == 0x01) // ip4
    {
        size += sizeof(resp.BindAddr.IPv4);
    }
    else if (resp.AddrType == 0x04) // ip6
    {
        size += sizeof(resp.BindAddr.IPv6);
    }
    else if (resp.AddrType == 0x03) // domain name
    {
        size += sizeof(resp.BindAddr.DomainLen) + resp.BindAddr.DomainLen;
    }
    size += sizeof(resp.BindPort);
    arr.resize(size);

    char *p = arr.data();
    int len_first_4 = sizeof(resp.Version) + sizeof(resp.Reply) + sizeof(resp.Reserved) + sizeof(resp.AddrType);
    memcpy(p, &resp, len_first_4);

    if (resp.AddrType == 0x01) // ip4
    {
        memcpy(p + len_first_4, &resp.BindAddr.IPv4, sizeof(resp.BindAddr.IPv4));
        memcpy(p + len_first_4 + sizeof(resp.BindAddr.IPv4), &resp.BindPort, sizeof(resp.BindPort));
    }
    else if (resp.AddrType == 0x04) // ip6
    {
        memcpy(p + len_first_4, &resp.BindAddr.IPv6, sizeof(resp.BindAddr.IPv6));
        memcpy(p + len_first_4 + sizeof(resp.BindAddr.IPv6), &resp.BindPort, sizeof(resp.BindPort));
    }
    else if (resp.AddrType == 0x03) // domain name
    {
        memcpy(p + len_first_4, &resp.BindAddr.DomainLen, sizeof(resp.BindAddr.DomainLen));
        memcpy(p + len_first_4 + sizeof(resp.BindAddr.DomainLen), &resp.BindAddr.Domain[0], resp.BindAddr.DomainLen);
        memcpy(p + len_first_4 + sizeof(resp.BindAddr.DomainLen) + resp.BindAddr.DomainLen, &resp.BindPort, sizeof(resp.BindPort));
    }

    return arr;
}


} // namespace SocksProxyServer
