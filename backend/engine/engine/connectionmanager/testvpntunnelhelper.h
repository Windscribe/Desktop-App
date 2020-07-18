#ifndef TESTVPNTUNNELHELPER_H
#define TESTVPNTUNNELHELPER_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QMutex>
#include <QHostInfo>
#include "Engine/Types/types.h"

class ServerAPI;

// helper class for TestVPNTunnel
class TestVPNTunnelHelper : public QObject
{
    Q_OBJECT
public:
    explicit TestVPNTunnelHelper(QObject *parent, ServerAPI *serverAPI);
    virtual ~TestVPNTunnelHelper();

public:
    void startTests(int testNum);
    void stopTests();

signals:
    void testsFinished(bool bSuccess, int testNum, const QString &ipAddress);

private slots:
    void onPingTestAnswer(SERVER_API_RET_CODE retCode, const QString &data);

private:
    enum { STATE_NONE, STATE_PING_REQUEST };

    ServerAPI *serverAPI_;
    int state_;
    qint64 sentPingCmdId_;
    bool bStopped_;
    int testNum_;

    static quint64 cmdId_;
    static QMutex mutexCmdId_;
};

#endif // TESTVPNTUNNELHELPER_H
