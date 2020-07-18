#ifndef SESSIONSTATUSTIMER_H
#define SESSIONSTATUSTIMER_H

#include <QObject>
#include "ConnectStateController/iconnectstatecontroller.h"

class SessionStatusTimer : public QObject
{
    Q_OBJECT
public:
    explicit SessionStatusTimer(QObject *parent, IConnectStateController *connectStateController);

    void applicationActivated();
    void applicationDeactivated();

    void start(int msec);
    void stop();


signals:
    void needUpdateRightNow();

private slots:
    void onTimer();
    void onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECTION_ERROR err, const LocationID &location);

private:

    const int TIMER_INTERVAL = 1000;

    bool isStarted_;
    int msec_;
    bool isApplicationActivated_;
    bool isConnected_;
    QTimer timer_;
    qint64 lastSignalEmitTimeMs_;
};

#endif // SESSIONSTATUSTIMER_H
