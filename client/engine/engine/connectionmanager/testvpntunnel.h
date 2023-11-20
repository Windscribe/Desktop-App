#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QTime>
#include <QVector>
#include "types/protocol.h"
#include "engine/serverapi/serverapi.h"

// do set of tests after VPN tunnel is established
class TestVPNTunnel : public QObject
{
    Q_OBJECT
public:
    explicit TestVPNTunnel(QObject *parent, server_api::ServerAPI *serverAPI);
    virtual ~TestVPNTunnel();

public slots:
    void startTests(const types::Protocol &protocol);
    void stopTests();

signals:
    void testsFinished(bool bSuccess, const QString &ipAddress);

private slots:
    void onPingTestAnswer();
    void doNextPingTest();
    void startTestImpl();
    void onTestsSkipped();

private:
    server_api::ServerAPI *serverAPI_;
    bool bRunning_;
    int curTest_;
    quint64 cmdId_;
    QElapsedTimer elapsed_;
    QTime lastTimeForCallWithLog_;
    int testRetryDelay_;
    bool doCustomTunnelTest_;
    QElapsedTimer elapsedOverallTimer_;

    QVector<uint> timeouts_;

    types::Protocol protocol_;

    server_api::BaseRequest *curRequest_;

};
