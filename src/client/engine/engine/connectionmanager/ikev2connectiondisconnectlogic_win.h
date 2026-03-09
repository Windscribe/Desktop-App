#pragma once

#include <QThread>
#include <windows.h>
#include <ras.h>
#include <raserror.h>

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

private:
    HRASCONN connHandle_;
};
