#ifndef SOCKSPROXYCONNECTION_H
#define SOCKSPROXYCONNECTION_H

#include <QObject>
#include <QTcpSocket>

#include "socksstructs.h"
#include "socksproxyreadexactly.h"
#include "socksproxyidentreqparser.h"
#include "../SocketUtils/socketwriteall.h"
#include "socksproxycommandparser.h"

namespace SocksProxyServer {

class SocksProxyConnection : public QObject
{
    Q_OBJECT
public:
    explicit SocksProxyConnection(qintptr socketDescriptor, const QString &hostname, QObject *parent = nullptr);

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

    enum { READ_IDENT_REQ, READ_COMMANDS, CONNECT_TO_HOST, RELAY_BETWEEN_CLIENT_SERVER } state_;

    QByteArray socketReadArr_;
    SocketWriteAll *writeAllSocket_;
    SocketWriteAll *writeAllSocketExternal_;

    SocksProxyIdentReqParser identReqParser_;
    SocksProxyCommandParser commandParser_;
    QScopedPointer<SocksProxyReadExactly> readExactly_;

    bool bAlreadyClosedAndEmitFinished_;

    QByteArray getByteArrayFromSocks5Resp(const socks5_resp &resp);

};

} // namespace SocksProxyServer

#endif // SOCKSPROXYCONNECTION_H
