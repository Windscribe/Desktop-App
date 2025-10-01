#pragma once

#include <QObject>
#include <QTimer>
#include <wsnet/WSNet.h>
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "types/locationid.h"

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

    void doDnsRequest();
    void onDnsRequestFinished(std::uint64_t requestId, const std::string &hostname, std::shared_ptr<wsnet::WSNetDnsRequestResult> result);
    void onPingFinished(const std::string &ip, bool isSuccess, std::int32_t timeMs, bool isFromDisconnectedVpnState);
};
