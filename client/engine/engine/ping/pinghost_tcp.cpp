#include "pinghost_tcp.h"
#include <QTcpSocket>
#include <QTimer>

PingHost_TCP::PingHost_TCP(QObject *parent, const QString &ip, const types::ProxySettings &proxySettings, bool isProxyEnabled) :
    IPingHost(parent), ip_(ip), proxySettings_(proxySettings), isProxyEnabled_(isProxyEnabled)
{
}

void PingHost_TCP::ping()
{
    tcpSocket_ = new QTcpSocket(this);
    if (isProxyEnabled())
    {
        tcpSocket_->setProxy(proxySettings_.getNetworkProxy());
    }
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &PingHost_TCP::onSocketTimeout);
    connect(tcpSocket_, &QTcpSocket::connected, this, &PingHost_TCP::onSocketConnected);
    connect(tcpSocket_, &QTcpSocket::bytesWritten, this, &PingHost_TCP::onSocketBytesWritten);
    connect(tcpSocket_, &QTcpSocket::errorOccurred, this, &PingHost_TCP::onSocketError);

    elapsedTimer_.start();
    timer_->setSingleShot(true);
    timer_->start(PING_TIMEOUT);
    tcpSocket_->connectToHost(ip_, 443, QTcpSocket::ReadWrite, QTcpSocket::IPv4Protocol);
}

void PingHost_TCP::onSocketConnected()
{
    if (tcpSocket_->write(".", 1) != 1)
        emit pingFinished(false, 0, ip_);
}

void PingHost_TCP::onSocketBytesWritten(qint64 bytes)
{
    if (bytes == 1)
        emit pingFinished(true, elapsedTimer_.elapsed(), ip_);
    else
        emit pingFinished(false, 0, ip_);
}

void PingHost_TCP::onSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    emit pingFinished(false, 0, ip_);
}

void PingHost_TCP::onSocketTimeout()
{
    emit pingFinished(false, 0, ip_);
}

bool PingHost_TCP::isProxyEnabled() const
{
    return (isProxyEnabled_ && proxySettings_.option() != PROXY_OPTION_NONE);
}
