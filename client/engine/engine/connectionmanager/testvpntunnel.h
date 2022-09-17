#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QTime>
#include <QVector>
#include "types/enums.h"

class ServerAPI;

// do set of tests after VPN tunnel is established
class TestVPNTunnel : public QObject
{
    Q_OBJECT
public:
    explicit TestVPNTunnel(QObject *parent, ServerAPI *serverAPI);
    virtual ~TestVPNTunnel();

public slots:
    void startTests(const PROTOCOL &protocol);
    void stopTests();

signals:
    void testsFinished(bool bSuccess, const QString &ipAddress);

private slots:
    void onPingTestAnswer(SERVER_API_RET_CODE retCode, const QString &data);
    void doNextPingTest();
    void startTestImpl();
    void onTestsSkipped();

private:
    ServerAPI *serverAPI_;
    bool bRunning_;
    int curTest_;
    quint64 cmdId_;
    QElapsedTimer elapsed_;
    QTime lastTimeForCallWithLog_;
    int testRetryDelay_;
    bool doCustomTunnelTest_;
    QElapsedTimer elapsedOverallTimer_;

    enum {
           PING_TEST_TIMEOUT_1 = 2000,
           PING_TEST_TIMEOUT_2 = 4000,
           PING_TEST_TIMEOUT_3 = 8000
       };
    QVector<uint> timeouts_;

    PROTOCOL protocol_;

};
