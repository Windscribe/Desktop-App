#ifndef GETMYIPCONTROLLER_H
#define GETMYIPCONTROLLER_H

#include <QObject>
#include <QQueue>
#include <QTimer>
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "engine/serverapi/serverapi.h"

//TODO: move to resources manager
class GetMyIPController : public QObject
{
    Q_OBJECT
public:
    explicit GetMyIPController(QObject *parent, server_api::ServerAPI *serverAPI, INetworkDetectionManager *networkDetectionManager);

    void getIPFromConnectedState(int timeoutMs);
    void getIPFromDisconnectedState(int timeoutMs);

signals:
    void answerMyIP(const QString &ip, bool isDisconnected);

private slots:
    void onTimer();
    void onMyIpAnswer();

private:
    static constexpr int kTimeout = 5000;

    server_api::ServerAPI *serverAPI_;
    INetworkDetectionManager *networkDetectionManager_;

    server_api::BaseRequest *curRequest_;

    bool requestForTimerIsDisconnected_;
    QTimer timer_;
};

#endif // GETMYIPCONTROLLER_H
