#pragma once

#include <QVector>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QElapsedTimer>
#include "ipinghost.h"
#include "types/proxysettings.h"

class PingHost_TCP : public IPingHost
{
    Q_OBJECT
public:
    explicit PingHost_TCP(QObject *parent, const QString &ip, const types::ProxySettings &proxySettings, bool isProxyEnabled);

    void ping() override;

private slots:
    void onSocketConnected();
    void onSocketBytesWritten(qint64 bytes);
    void onSocketError(QAbstractSocket::SocketError socketError);
    void onSocketTimeout();

private:
    enum {PING_TIMEOUT = 2000};

    QString ip_;
    types::ProxySettings proxySettings_;
    bool isProxyEnabled_ = false;
    QTcpSocket *tcpSocket_ = nullptr;
    QTimer *timer_ = nullptr;
    QElapsedTimer elapsedTimer_;

    bool isProxyEnabled() const;
    void processError();
};

