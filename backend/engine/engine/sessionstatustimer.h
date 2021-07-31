#ifndef SESSIONSTATUSTIMER_H
#define SESSIONSTATUSTIMER_H

#include <QObject>
#include "connectstatecontroller/iconnectstatecontroller.h"

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
    void onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, ProtoTypes::ConnectError err, const LocationID &location);

private:

    static constexpr int TIMER_INTERVAL = 1000;

    bool isStarted_;
    int msec_;
    bool isApplicationActivated_;
    bool isConnected_;
    QTimer timer_;
    qint64 lastSignalEmitTimeMs_;
};

#endif // SESSIONSTATUSTIMER_H
