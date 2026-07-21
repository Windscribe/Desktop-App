#pragma once

#include <QScopedPointer>

#include "engine/connectionmanager/connectors/wireguard/wireguardconnectionbase.h"
#include "engine/helper/helper.h"
#include "wireguardringlogger.h"
#include "utils/win32handle.h"

class WireGuardConnection : public WireGuardConnectionBase
{
    Q_OBJECT

public:
    WireGuardConnection(QObject *parent, Helper *helper, types::Protocol protocol, const WireGuardSessionParams &sessionParams);
    ~WireGuardConnection() override;

    void startConnect() override;
    void startDisconnect() override;
    bool isDisconnected() const override;

protected:
    void run() override;

private slots:
    void onCheckServiceRunning();
    void onGetWireguardLogUpdates();
    void onGetWireguardStats();

private:
    static constexpr int kTimeoutForGetStats     = 5000;  // 5s timeout for the requesting send/recv stats from helper
    static constexpr int kTimeoutForCheckService = 5000;  // 5s timeout for the checking if the WG service is running
    static constexpr int kTimeoutForLogUpdate    = 250;   // 250ms timeout for getting log updates from the ring logger

    Helper* const helper_;
    WireGuardConfig wireGuardConfig_;

    bool connectedSignalEmited_ = false;

    QScopedPointer< wsl::WireguardRingLogger > wireguardLog_;
    wsl::Win32Handle stopThreadEvent_;

private:
    bool isAmneziaWG() const;
    quint64 lastHandshake() const;
    void onTunnelConnected();
    void onWireguardHandshakeFailure();
    void resetLogReader();
    bool startService();
    void stopService();
    void stop();

    static void CALLBACK checkServiceRunningProc(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue);
    static void CALLBACK getWireguardLogUpdatesProc(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue);
    static void CALLBACK getWireguardStatsProc(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue);
};
