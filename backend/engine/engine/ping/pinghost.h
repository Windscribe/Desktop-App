#ifndef PINGHOST_H
#define PINGHOST_H

#include <QObject>
#include "pinghost_tcp.h"

#ifdef Q_OS_WIN
    #include "pinghost_icmp_win.h"
#else
    #include "pinghost_icmp_mac.h"
#endif

// wrapper for PingHost_TCP and PingHost_ICMP
class PingHost : public QObject
{
    Q_OBJECT

public:
    enum PING_TYPE { PING_TCP, PING_ICMP };

    explicit PingHost(QObject *parent, IConnectStateController *stateController);

    void addHostForPing(const QString &ip, PING_TYPE pingType);
    void clearPings();

    void setProxySettings(const ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

signals:
    void pingFinished(bool bSuccess, int timems, const QString &ip, bool isFromDisconnectedState);

private slots:
    void addHostForPingImpl(const QString &ip, PingHost::PING_TYPE pingType);
    void clearPingsImpl();

    void setProxySettingsImpl(const ProxySettings &proxySettings);
    void disableProxyImpl();
    void enableProxyImpl();

private:
    PingHost_TCP pingHostTcp_;
#ifdef Q_OS_WIN
    PingHost_ICMP_win pingHostIcmp_;
#else
    PingHost_ICMP_mac pingHostIcmp_;
#endif

};

#endif // PINGHOST_H
