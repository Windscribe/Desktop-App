#pragma once

#include <QHostAddress>
#include <QObject>
#include <QTcpSocket>
#include <memory>
#include <wsnet/WSNet.h>
#include "httpproxyrequestparser.h"
#include "httpproxywebanswerparser.h"
#include "httpproxyreply.h"
#include "../proxyauthconfig.h"
#include "../socketutils/socketwriteall.h"

namespace HttpProxyServer {

class HttpProxyConnection : public QObject
{
    Q_OBJECT
public:
    explicit HttpProxyConnection(qintptr socketDescriptor, const QString &hostname,
                                 const ProxyAuth::Config &auth, QObject *parent = nullptr);

    bool start(qintptr socketDescriptor);

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

    enum { READ_CLIENT_REQUEST, RESOLVING_DESTINATION, CONNECTING_TO_EXTERNAL_SERVER, RELAY_BETWEEN_CLIENT_SERVER, READ_HEADERS_FROM_WEBSERVER, STATE_WRITE_HTTP_ERROR } state_;
    HttpProxyRequestParser requestParser_;
    HttpProxyWebAnswerParser webAnswerParser_;


    SocketWriteAll *writeAllSocket_;
    SocketWriteAll *writeAllSocketExternal_;

    QByteArray extraContent_;
    HttpProxyReply httpError_;

    std::shared_ptr<wsnet::WSNetCancelableCallback> dnsLookupCancelable_;

    bool bAlreadyClosedAndEmitFinished_;
    quint64 preRelayBytes_ = 0;
    void closeSocketsAndEmitFinished();

    bool sendChallenge();
    void writeError(HttpProxyReply::status_type status);
    void onDestinationResolved(const QList<QHostAddress> &allowedAddresses);
};

} // namespace HttpProxyServer
