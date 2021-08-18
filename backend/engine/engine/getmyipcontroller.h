#ifndef GETMYIPCONTROLLER_H
#define GETMYIPCONTROLLER_H

#include <QObject>
#include <QQueue>
#include <QTimer>
#include "engine/networkstatemanager/inetworkstatemanager.h"

class ServerAPI;

class GetMyIPController : public QObject
{
    Q_OBJECT
public:
    explicit GetMyIPController(QObject *parent, ServerAPI *serverAPI, INetworkStateManager *networkStateManager);

    void getIPFromConnectedState(int timeoutMs);
    void getIPFromDisconnectedState(int timeoutMs);

signals:
    void answerMyIP(const QString &ip, bool success, bool isDisconnected);

private slots:
    void onTimer();
    void onMyIpAnswer(const QString &ip, bool success, bool isDisconnected, uint userRole);

private:
    ServerAPI *serverAPI_;
    INetworkStateManager *networkStateManager_;
    uint serverApiUserRole_;

    bool requestForTimerIsDisconnected_;
    QTimer timer_;
};

#endif // GETMYIPCONTROLLER_H
