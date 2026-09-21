#pragma once

#include <QHostAddress>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <memory>
#include <wsnet/WSNet.h>
#include "../proxyauthconfig.h"
#include "../socketutils/socketwriteall.h"
#include "httpproxyheader.h"      // for HttpProxyHeader (used in auth challenge)
#include "httpproxyreply.h"
#include "httpproxyrequestparser.h"
#include "httpproxywebanswerparser.h"

class TestProxyServers;

namespace HttpProxyServer {

class HttpProxyConnection : public QObject
{
    Q_OBJECT
public:
    explicit HttpProxyConnection(qintptr socketDescriptor, const QString &hostname,
                                 const ProxyAuth::Config &auth, QObject *parent = nullptr);

public slots:
    void start();
    void forceClose();

signals:
    void finished(const QString &hostname);

private slots:
    void onSocketDisconnected();
    void onSocketReadyRead();

    void onSocketAllDataWritten();

    void onExternalSocketConnected();
    void onExternalSocketDisconnected();
    void onExternalSocketReadyRead();
    void onExternalSocketError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket *socket_;
    QTcpSocket *socketExternal_;
    qintptr socketDescriptor_;
    QString hostname_;
    ProxyAuth::Config auth_;

    const char *reply_established_ = "HTTP/1.0 200 Connection established\r\nProxy-agent: " WS_PRODUCT_NAME "\r\n\r\n";

    enum State {
        READ_CLIENT_REQUEST, RESOLVING_DESTINATION, CONNECTING_TO_EXTERNAL_SERVER,
        RELAY_BETWEEN_CLIENT_SERVER, RELAY_DRAINING, READ_HEADERS_FROM_WEBSERVER, STATE_WRITE_HTTP_ERROR
    };
    State state_;
    HttpProxyRequestParser requestParser_;
    HttpProxyWebAnswerParser webAnswerParser_;


    SocketWriteAll *writeAllSocket_;
    SocketWriteAll *writeAllSocketExternal_;

    QByteArray extraContent_;
    HttpProxyReply httpError_;

    std::shared_ptr<wsnet::WSNetCancelableCallback> dnsLookupCancelable_;

    bool bAlreadyClosedAndEmitFinished_;
    quint64 preRelayBytes_ = 0;
    QTimer *phaseTimer_ = nullptr;
    static int phaseTimeoutMs_;

    friend class ::TestProxyServers;

    void closeSocketsAndEmitFinished();
    void setState(State newState);
    void relayClientToUpstream();
    void relayUpstreamToClient();

    bool sendChallenge();
    void writeError(HttpProxyReply::status_type status);
    void onDestinationResolved(const QList<QHostAddress> &allowedAddresses);
};

} // namespace HttpProxyServer
