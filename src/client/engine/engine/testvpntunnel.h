#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QTime>
#include <QVector>
#include <wsnet/WSNet.h>
#include "types/protocol.h"

// do set of tests after VPN tunnel is established
class TestVPNTunnel : public QObject
{
    Q_OBJECT
public:
    explicit TestVPNTunnel(QObject *parent);
    virtual ~TestVPNTunnel();

public slots:
    void startTests(const types::Protocol &protocol = types::Protocol());
    void stopTests();

signals:
    void testsFinished(bool bSuccess, const QString &ipAddress);

private slots:
    void onPingTestAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &ipAddress);
    void doNextPingTest();
    void startTestImpl();
    void onTestsSkipped();

private:
    bool bRunning_;
    int curTest_;
    QElapsedTimer elapsed_;
    QTime lastTimeForCallWithLog_;
    int testRetryDelay_;
    bool doCustomTunnelTest_;
    QElapsedTimer elapsedOverallTimer_;

    QVector<uint> timeouts_;

    types::Protocol protocol_;

    std::shared_ptr<wsnet::WSNetCancelableCallback> curRequest_;

    std::shared_ptr<wsnet::WSNetCancelableCallback> callPingTest(std::uint32_t timeoutMs);
};
