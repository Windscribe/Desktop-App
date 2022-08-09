#ifndef SESSIONSTATUSTIMER_H
#define SESSIONSTATUSTIMER_H

#include <QDateTime>
#include <QObject>

#include "connectstatecontroller/iconnectstatecontroller.h"

class SessionStatusTimer : public QObject
{
    Q_OBJECT
public:
    explicit SessionStatusTimer(QObject *parent, IConnectStateController *connectStateController);

    void applicationActivated();
    void applicationDeactivated();

    void start();
    void stop();

signals:
    void needUpdateRightNow();

private slots:
    void onTimer();
    void onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &location);

private:
    static constexpr int TIMER_INTERVAL = 60000;

    QTimer timer_;
    bool isConnected_ = false;
    QDateTime lastSessionUpdateRequest_;

private:
    void requestSessionUpdate();
};

#endif // SESSIONSTATUSTIMER_H
