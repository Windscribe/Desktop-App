#pragma once

#include <QObject>
#include <wsnet/WSNet.h>

// Qt-side handle for the tunnel test wsnet runs after the VPN tunnel is established.
class TestVPNTunnel : public QObject
{
    Q_OBJECT
public:
    explicit TestVPNTunnel(QObject *parent);
    virtual ~TestVPNTunnel();

public slots:
    void startTests(bool isBridgeAllowed);
    void stopTests();

signals:
    void testsFinished(bool bSuccess, const QString &ipAddress);

private:
    std::shared_ptr<wsnet::WSNetCancelableCallback> curRequest_;
    // Tells the current run apart from one already stopped, whose result can still be queued in the
    // event loop when the next run starts.
    quint64 runId_ = 0;

    void onTestFinished(quint64 runId, bool isSuccess, const std::string &ipv4, const std::string &ipv6);
};
