#pragma once

#include <QObject>
#include <QTimer>

#include "types/locationid.h"
#include "pingmultiplehosts.h"

// If enabled, when user is connected to the tunnel send an ICMP request to windscribe.com every 10s.
class KeepAliveManager : public QObject
{
    Q_OBJECT
public:
    explicit KeepAliveManager(QObject *parent, IConnectStateController *stateController, NetworkAccessManager *networkAccessManager);

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

    PingMultipleHosts pingHosts_;
};
