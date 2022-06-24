#ifndef TESTVPNTUNNEL_H
#define TESTVPNTUNNEL_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QTime>
#include <QVector>
#include "engine/types/types.h"
#include "engine/types/protocoltype.h"

#if defined(Q_OS_WINDOWS)
#include <windows.h>
#include <WinDNS.h>
#endif

class ServerAPI;

#if defined(Q_OS_WINDOWS)
class DnsQueryContext;
#endif

// do set of tests after VPN tunnel is established
class TestVPNTunnel : public QObject
{
    Q_OBJECT
public:
    explicit TestVPNTunnel(QObject *parent, ServerAPI *serverAPI);
    virtual ~TestVPNTunnel();

public slots:
    void startTests(const ProtocolType &protocol);
    void stopTests();

signals:
    void testsFinished(bool bSuccess, const QString &ipAddress);

private slots:
    void onPingTestAnswer(SERVER_API_RET_CODE retCode, const QString &data);
    void doNextPingTest();
    void startTestImpl();
    void onTestsSkipped();

    #if defined(Q_OS_WINDOWS)
    bool initiateWin32TunnelTest();
    void onWin32DnsQueryCompleted();
    void onWin32DnsQueryTimeout();
    #endif

private:
    ServerAPI *serverAPI_;
    bool bRunning_;
    int curTest_;
    quint64 cmdId_;
    QElapsedTimer elapsed_;
    QTime lastTimeForCallWithLog_;
    int testRetryDelay_;
    bool doCustomTunnelTest_;

    enum {
           PING_TEST_TIMEOUT_1 = 2000,
           PING_TEST_TIMEOUT_2 = 4000,
           PING_TEST_TIMEOUT_3 = 8000
       };
    QVector<uint> timeouts_;

    bool doWin32TunnelTest_;
    ProtocolType protocol_;

    #if defined(Q_OS_WINDOWS)
    typedef DNS_STATUS WINAPI DnsQueryEx_T(PDNS_QUERY_REQUEST pQueryRequest, PDNS_QUERY_RESULT pQueryResults, PDNS_QUERY_CANCEL pCancelHandle);
    typedef DNS_STATUS WINAPI DnsCancelQuery_T(PDNS_QUERY_CANCEL pCancelHandle);
    DnsQueryEx_T* DnsQueryEx_f;
    DnsCancelQuery_T* DnsCancelQuery_f;
    HMODULE dllHandle_;
    QScopedPointer< DnsQueryContext > dnsQueryContext_;
    std::wstring serverTunnelTestUrl_;
    QTimer dnsQueryTimeout_;

    static VOID WINAPI DnsQueryCompleteCallback(_In_ PVOID Context, _Inout_ PDNS_QUERY_RESULT QueryResults);

    bool loadWinDnsApiEndpoints();
    #endif
};

#endif // TESTVPNTUNNEL_H
