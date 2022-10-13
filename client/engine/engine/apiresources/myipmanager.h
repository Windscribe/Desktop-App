#pragma once

#include <QObject>
#include <QQueue>
#include <QTimer>
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/serverapi/serverapi.h"

namespace api_resources {

class MyIpManager : public QObject
{
    Q_OBJECT
public:
    explicit MyIpManager(QObject *parent, server_api::ServerAPI *serverAPI, INetworkDetectionManager *networkDetectionManager, IConnectStateController *connectStateController);

    void getIP(int timeoutMs);

signals:
    void myIpChanged(const QString &ip, bool isFromDisconnectedState);

private slots:
    void onTimer();
    void onMyIpAnswer();
    void onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &location);

private:
    static constexpr int kTimeout = 5000;

    server_api::ServerAPI *serverAPI_;
    INetworkDetectionManager *networkDetectionManager_;
    IConnectStateController *connectStateController_;

    server_api::BaseRequest *curRequest_;
    bool requestForTimerIsDisconnected_;
    QTimer timer_;
};

} // namespace api_resources
