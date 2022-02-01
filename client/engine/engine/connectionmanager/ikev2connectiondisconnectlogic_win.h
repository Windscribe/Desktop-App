#ifndef IKEV2CONNECTIONDISCONNECTLOGIC_WIN_H
#define IKEV2CONNECTIONDISCONNECTLOGIC_WIN_H

#include <QThread>
#include <windows.h>
#include <ras.h>
#include <raserror.h>
#include <QTimer>
#include <QElapsedTimer>
#include <QWaitCondition>
#include <QMutex>

class IKEv2ConnectionDisconnectLogic_win : public QThread
{
    Q_OBJECT
public:
    explicit IKEv2ConnectionDisconnectLogic_win(QObject *parent);

    void startDisconnect(HRASCONN connHandle);
    bool isDisconnected();

    static void blockingDisconnect(HRASCONN connHandle);

signals:
    void disconnected();

protected:
    virtual void run();

private slots:
    void onTimer();

private:
    int cntRasHangUp_;
    HRASCONN connHandle_;
    QTimer timer_;
    QElapsedTimer elapsedTimer_;
    QWaitCondition waitCondition_;
    QMutex mutex_;

    void waitForControlThreadFinish();
};

#endif // IKEV2CONNECTIONDISCONNECTLOGIC_WIN_H
