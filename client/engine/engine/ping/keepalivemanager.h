#pragma once

#include <QObject>

#ifdef Q_OS_WIN
    #include "pinghost_icmp_win.h"
#elif defined (Q_OS_MAC) || defined(Q_OS_LINUX)
    #include "pinghost_icmp_mac.h"
#endif

// If enabled, when user is connected to the tunnel send an ICMP request to windscribe.com every 10s.
class KeepAliveManager : public QObject
{
    Q_OBJECT
public:
    explicit KeepAliveManager(QObject *parent, IConnectStateController *stateController);

    void setEnabled(bool isEnabled);

private slots:
    void onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &location);
    void onTimer();
    void onDnsRequestFinished();
    void onPingFinished(bool bSuccess, int timems, const QString &ip, bool isFromDisconnectedState);

private:
    bool isEnabled_;
    CONNECT_STATE curConnectState_;
    static constexpr int KEEP_ALIVE_TIMEOUT = 10000;
    QTimer timer_;

    struct IP_DESCR
    {
        QString ip_;
        bool bFailed_;

        IP_DESCR() : bFailed_(false) {}
        explicit IP_DESCR(QString ip): ip_(ip), bFailed_(false) {}
    };

    QVector<IP_DESCR> ips_;

#ifdef Q_OS_WIN
    PingHost_ICMP_win pingHostIcmp_;
#elif defined (Q_OS_MAC) || defined(Q_OS_LINUX)
    PingHost_ICMP_mac pingHostIcmp_;
#endif
};
