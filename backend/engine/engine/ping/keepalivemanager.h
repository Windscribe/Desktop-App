#ifndef KEEPALIVEMANAGER_H
#define KEEPALIVEMANAGER_H

#include <QObject>
#include <QHostInfo>
#include "pinghost_tcp.h"

#ifdef Q_OS_WIN
    #include "pinghost_icmp_win.h"
#else
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
    void onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECTION_ERROR err, const LocationID &location);
    void onTimer();
    void onDnsResolvedFinished(const QString &hostname, const QHostInfo &hostInfo, void *userPointer, quint64 userId);
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
#else
    PingHost_ICMP_mac pingHostIcmp_;
#endif
};

#endif // KEEPALIVEMANAGER_H
