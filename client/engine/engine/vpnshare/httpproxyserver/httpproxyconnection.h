#ifndef HTTPPROXYCONNECTION_H
#define HTTPPROXYCONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include "httpproxyrequestparser.h"
#include "httpproxywebanswerparser.h"
#include "httpproxyreply.h"
#include "../socketutils/socketwriteall.h"

namespace HttpProxyServer {

class HttpProxyConnection : public QObject
{
    Q_OBJECT
public:
    explicit HttpProxyConnection(qintptr socketDescriptor, const QString &hostname, QObject *parent = nullptr);

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

    const char *reply_established_ = "HTTP/1.0 200 Connection established\r\nProxy-agent: Windscribe\r\n\r\n";

    enum { READ_CLIENT_REQUEST, CONNECTING_TO_EXTERNAL_SERVER, RELAY_BETWEEN_CLIENT_SERVER, READ_HEADERS_FROM_WEBSERVER, STATE_WRITE_HTTP_ERROR } state_;
    HttpProxyRequestParser requestParser_;
    HttpProxyWebAnswerParser webAnswerParser_;


    SocketWriteAll *writeAllSocket_;
    SocketWriteAll *writeAllSocketExternal_;

    QByteArray extraContent_;
    HttpProxyReply httpError_;

    bool bAlreadyClosedAndEmitFinished_;
    void closeSocketsAndEmitFinished();
};

} // namespace HttpProxyServer

#endif // HTTPPROXYCONNECTION_H
