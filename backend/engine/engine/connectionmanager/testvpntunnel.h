#ifndef TESTVPNTUNNEL_H
#define TESTVPNTUNNEL_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QTime>
#include <QVector>
#include "engine/types/types.h"

class ServerAPI;

// do set of tests after VPN tunnel is established
class TestVPNTunnel : public QObject
{
    Q_OBJECT
public:
    explicit TestVPNTunnel(QObject *parent, ServerAPI *serverAPI);

public slots:
    void startTests();
    void stopTests();

signals:
    void testsFinished(bool bSuccess, const QString &ipAddress);

private slots:
    void onPingTestAnswer(SERVER_API_RET_CODE retCode, const QString &data);
    void doNextPingTest();

private:
    ServerAPI *serverAPI_;
    bool bRunning_;
    int curTest_;
    quint64 cmdId_;
    QElapsedTimer elapsed_;
    QTime lastTimeForCallWithLog_;

    enum {
           PING_TEST_TIMEOUT_1 = 2000,
           PING_TEST_TIMEOUT_2 = 4000,
           PING_TEST_TIMEOUT_3 = 8000,
       };
    QVector<uint> timeouts_;
};

#endif // TESTVPNTUNNEL_H
