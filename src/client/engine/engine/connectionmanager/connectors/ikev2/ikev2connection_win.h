#pragma once

#include <windows.h>
#include <ras.h>
#include <raserror.h>

#include <QString>
#include <QVector>

#include "engine/connectionmanager/connectors/ikev2/ikev2connectionbase.h"
#include "engine/helper/helper.h"
#include "utils/win32handle.h"

// IKEv2 connector for Windows.
//
// Threading model:
//  - startConnect()/startDisconnect()/isDisconnected()/waitForDisconnect()/the destructor run on the
//    engine thread.  They only touch the (thread-safe) Win32 event objects and QThread start/wait/
//    isRunning().
//  - run(), the APC body (handleDialState), and every helper_/RAS call run on the worker thread.
//  - rasDialCallbackProc runs on a RAS-owned thread and does nothing but queue an APC to the worker thread.
//
// Resilience: MOBIKE is left enabled (RASEO2_DisableMobility is NOT set) and dwNetworkOutageTime is
// configured so the IKEv2 SA survives brief network changes/outages transparently -- without firing
// RasConnectionNotification and without involving the engine.  True reconnects/failover remain the
// engine's responsibility (ConnectionManager).

class IKEv2Connection_win : public Ikev2ConnectionBase
{
    Q_OBJECT
public:
    explicit IKEv2Connection_win(QObject *parent, Helper *helper, types::Protocol protocol, const Ikev2SessionParams &sessionParams);
    ~IKEv2Connection_win() override;

    void startConnect() override;
    void startDisconnect() override;
    bool isDisconnected() const override;
    void waitForDisconnect() override;

    // Used outside of an active connection, for app startup/shutdown cleanup of orphaned connections.
    static void removeIkev2ConnectionFromOS();
    static QVector<HRASCONN> getActiveIkev2Connections();

    // Synchronously hangs up the given RAS connection and waits (bounded) for it to be torn down.
    static void blockingDisconnect(HRASCONN connHandle);

protected:
    void run() override;

private:
    // Terminal decision the dial callback (running on the worker thread, via an APC) hands to the
    // monitor loop.  Only the worker thread ever touches it, so it needs no synchronization.  The
    // connected transition is applied inline in the APC and so has no entry here.
    enum class PendingResult { None, AuthError, ReinstallWan, ConnectFailed };

    // Outcome of a single dial attempt's monitor loop.
    enum class MonitorResult { Stop, Dropped, AuthError, ReinstallWanRequested, Failed };

    // Outcome of a WAN-reinstall recovery step.
    enum class ReinstallResult { Retry, TerminalError, Stopped };

    // Outcome of the synchronous dial setup in startDial().
    enum class DialResult { Started, Error, Stopped };

    Helper* const helper_;

    QString initialUrl_;
    QString initialIp_;
    QString initialUsername_;
    QString initialPassword_;
    bool initialEnableIkev2Compression_ = false;

    HRASCONN connHandle_ = NULL;
    PendingResult pendingResult_ = PendingResult::None;
    int cntFailedConnectionAttempts_ = 0;
    bool hostsAndDnsProtectionSet_ = false;
    bool connectedSignalEmited_ = false;

    wsl::Win32Handle workerThreadHandle_;
    wsl::Win32Handle stopThreadEvent_;
    wsl::Win32Handle notifyEvent_;

    static constexpr DWORD kTimeoutForGetStats          = 5000;
    static constexpr int   kMaxFailedConnectionAttempts = 3;
    static constexpr int   kWanReinstallPause           = 3000;
    // Keep the IKEv2 SA alive (via MOBIKE) across brief network outages, in minutes.  See the
    // RASENTRY.dwNetworkOutageTime documentation.
    static constexpr DWORD kNetworkOutageTimeMinutes = 1;

    static constexpr qint64 kBlockingDisconnectTimeoutMs = 8500;
    static constexpr qint64 kRasHangUpRetryIntervalMs    = 3000;
    static constexpr int    kMaxRasHangUpAttempts        = 3;
    static constexpr DWORD  kDisconnectPollIntervalMs    = 10;

    // static global, because we reinstall WAN only once during the lifetime of the process.
    static bool wanReinstalled_;

private:
    bool armDropDetection();
    void cleanupHostsAndDnsProtection();
    void emitStatistics();
    bool findIKEv2Device(RASDEVINFO *outDevInfo);
    ReinstallResult handleReinstallWan();
    MonitorResult monitorLoop();
    DialResult startDial();
    void stop();
    bool stopRequested() const;

    void handleDialState(HRASCONN connHandle, RASCONNSTATE rascs, DWORD dwError);
    static void CALLBACK rasDialApc(ULONG_PTR param);
    static DWORD CALLBACK rasDialCallbackProc(ULONG_PTR dwCallbackId, DWORD dwSubEntry, HRASCONN hrasconn,
                                              UINT unMsg, RASCONNSTATE rascs, DWORD dwError, DWORD dwExtendedError);

    static void blockingDisconnect(HRASCONN connHandle, bool alertable);
    static bool isRasDisconnected(HRASCONN connHandle);
    static bool isTransientConnectError(DWORD dwError);
    static QString rasConnStateToString(RASCONNSTATE state);
};
