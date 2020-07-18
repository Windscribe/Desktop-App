#ifndef TESTVPNTUNNEL_H
#define TESTVPNTUNNEL_H

#include <QObject>
#include <QTimer>

class ServerAPI;
class TestVPNTunnelHelper;

// do set of tests after VPN tunnel is established
class TestVPNTunnel : public QObject
{
    Q_OBJECT
public:
    explicit TestVPNTunnel(QObject *parent, ServerAPI *serverAPI);
    virtual ~TestVPNTunnel();

public slots:
    void startTests();
    void stopTests();

signals:
    void testsFinished(bool bSuccess, const QString &ipAddress);

private slots:
    void onHelperTestsFinished(bool bSuccess, int testNum, const QString &ipAddress);
    void onStartTestTimerTick();

private:
    bool bRunning_;
    TestVPNTunnelHelper *testVPNTunnelHelper_;
    ServerAPI *serverAPI_;
    enum {TIMER_JOB_TEST1 = 1, TIMER_JOB_TEST2, TIMER_JOB_TEST3};

    void deleteTestVpnTunnelHelper();
    void startHelperTest(int testNum);
    QTimer startTestTimer_;
};

#endif // TESTVPNTUNNEL_H
