#pragma once

#include <memory>

#include <QObject>
#include <QQueue>
#include <QTimer>
#include <wsnet/WSNet.h>
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/connectstatecontroller/connectstatewatcher.h"

namespace api_resources {

class MyIpManager : public QObject
{
    Q_OBJECT
public:
    explicit MyIpManager(QObject *parent, INetworkDetectionManager *networkDetectionManager, IConnectStateController *connectStateController);

    void getIP(int timeoutMs);

signals:
    void myIpChanged(const QString &ip, bool isFromDisconnectedState);

private slots:
    void onTimer();

private:
    static constexpr int kTimeout = 5000;

    INetworkDetectionManager *networkDetectionManager_;
    IConnectStateController *connectStateController_;

    std::shared_ptr<wsnet::WSNetCancelableCallback> curRequest_;
    std::unique_ptr<ConnectStateWatcher> connectStateWatcher_;
    quint64 curRequestId_ = 0;

    QTimer timer_;

    void onMyIpAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData, quint64 requestId, bool isFromDisconnectedState);
};

} // namespace api_resources
