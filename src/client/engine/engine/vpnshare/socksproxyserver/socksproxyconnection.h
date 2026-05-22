#pragma once

#include <QHostAddress>
#include <QList>
#include <QObject>
#include <QTcpSocket>
#include <memory>
#include <wsnet/WSNet.h>

#include "socksstructs.h"
#include "socksproxyreadexactly.h"
#include "socksproxyidentreqparser.h"
#include "../proxyauthconfig.h"
#include "../socketutils/socketwriteall.h"
#include "socksproxycommandparser.h"

namespace SocksProxyServer {

class SocksProxyConnection : public QObject
{
    Q_OBJECT
public:
    explicit SocksProxyConnection(qintptr socketDescriptor, const QString &hostname,
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

    /*void onSocketAllDataWritten();*/

    void onExternalSocketConnected();
    void onExternalSocketDisconnected();
    void onExternalSocketReadyRead();
    void onExternalSocketError(QAbstractSocket::SocketError socketError);
private slots:
    void closeSocketsAndEmitFinished();
private:
    QTcpSocket *socket_;
    QTcpSocket *socketExternal_;
    qintptr socketDescriptor_;
    QString hostname_;
    ProxyAuth::Config auth_;

    enum { READ_IDENT_REQ, READ_AUTH, READ_COMMANDS, RESOLVING_DESTINATION, CONNECT_TO_HOST, RELAY_BETWEEN_CLIENT_SERVER } state_;

    QByteArray socketReadArr_;
    SocketWriteAll *writeAllSocket_;
    SocketWriteAll *writeAllSocketExternal_;

    SocksProxyIdentReqParser identReqParser_;
    SocksProxyCommandParser commandParser_;
    QScopedPointer<SocksProxyReadExactly> readExactly_;

    std::shared_ptr<wsnet::WSNetCancelableCallback> dnsLookupCancelable_;

    bool bAlreadyClosedAndEmitFinished_;

    QByteArray getByteArrayFromSocks5Resp(const socks5_resp &resp);

    void handleIdentRequest();
    void handleAuthRequest();
    void handleCommandRequest();
    void connectExternal(const QHostAddress &addr, quint16 port);
    void sendReply(quint8 reply);

};

} // namespace SocksProxyServer
