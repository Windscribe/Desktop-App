#pragma once

#include <QScopedPointer>

#include "iconnection.h"
#include "engine/helper/helper_win.h"
#include "wireguardringlogger.h"
#include "utils/win32handle.h"

class WireGuardConnection : public IConnection
{
    Q_OBJECT

public:
    WireGuardConnection(QObject *parent, IHelper *helper);
    ~WireGuardConnection() override;

    void startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName,
                      const QString &username, const QString &password, const types::ProxySettings &proxySettings,
                      const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode) override;
    void startDisconnect() override;
    bool isDisconnected() const override;

    ConnectionType getConnectionType() const override { return ConnectionType::WIREGUARD; }

    void continueWithUsernameAndPassword(const QString & /*username*/, const QString & /*password*/) override {}
    void continueWithPassword(const QString & /*password*/) override {}

    static QString getWireGuardExeName() { return QString("WireguardService"); }
    static QString getWireGuardAdapterName() { return QString("WireGuardTunnel"); }

protected:
    void run() override;

private Q_SLOTS:
    void onCheckServiceRunning();
    void onGetWireguardLogUpdates();
    void onGetWireguardStats();
    void onAutomaticConnectionTimeout();

private:
    static constexpr int kTimeoutForGetStats     = 5000;  // 5s timeout for the requesting send/recv stats from helper
    static constexpr int kTimeoutForCheckService = 5000;  // 5s timeout for the checking if the WG service is running
    static constexpr int kTimeoutForLogUpdate    = 250;   // 250ms timeout for getting log updates from the ring logger
    static constexpr int kTimeoutForAutomatic    = 20000; // 20s timeout for the automatic connection mode

    Helper_win* const helper_;
    const WireGuardConfig* wireGuardConfig_ = nullptr;

    bool connectedSignalEmited_ = false;
    bool isAutomaticConnectionMode_ = false;

    QScopedPointer< wsl::WireguardRingLogger > wireguardLog_;
    wsl::Win32Handle stopThreadEvent_;

private:
    void onTunnelConnected();
    void onWireguardHandshakeFailure();
    bool startService();
    void stop();

    static void CALLBACK checkServiceRunningProc(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue);
    static void CALLBACK getWireguardLogUpdatesProc(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue);
    static void CALLBACK getWireguardStatsProc(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue);
    static void CALLBACK automaticConnectionTimeoutProc(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue);
};
