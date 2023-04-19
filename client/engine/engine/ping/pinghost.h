#pragma once

#include <QObject>
#include "pinghost_tcp.h"
#include "pinghost_curl.h"

#ifdef Q_OS_WIN
    #include "pinghost_icmp_win.h"
    #include "utils/crashhandler.h"
#else
    #include "pinghost_icmp_mac.h"
#endif

class NetworkAccessManager;

// wrapper for PingHost_TCP, PingHost_ICMP and PingHost_Curl
class PingHost : public QObject
{
    Q_OBJECT

public:
    enum PING_TYPE { PING_TCP, PING_ICMP, PING_CURL };

    explicit PingHost(QObject *parent, IConnectStateController *stateController, NetworkAccessManager *networkAccessManager);

    // hostname must be empty for PING_TCP and PING_ICMP
    void addHostForPing(const QString &ip, PING_TYPE pingType, const QString &hostname);
    void clearPings();

    void setProxySettings(const types::ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

signals:
    void pingFinished(bool success, int timems, const QString &ip, bool isFromDisconnectedState);

private slots:
    void addHostForPingImpl(const QString &ip, PingHost::PING_TYPE pingType, const QString &hostname);
    void clearPingsImpl();

    void setProxySettingsImpl(const types::ProxySettings &proxySettings);
    void disableProxyImpl();
    void enableProxyImpl();

private:
    PingHost_Curl pingHostCurl_;
    PingHost_TCP pingHostTcp_;
#ifdef Q_OS_WIN
    PingHost_ICMP_win pingHostIcmp_;
    QScopedPointer<Debug::CrashHandlerForThread> crashHandler_;
#else
    PingHost_ICMP_mac pingHostIcmp_;
#endif
};
