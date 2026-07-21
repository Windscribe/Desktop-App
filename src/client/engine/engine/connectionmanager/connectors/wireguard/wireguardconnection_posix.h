#pragma once

#include <atomic>
#include <QMutex>
#include <QTimer>
#include "engine/connectionmanager/connectors/wireguard/wireguardconnectionbase.h"
#include "engine/helper/helper.h"


class WireGuardConnectionImpl;

class WireGuardConnection : public WireGuardConnectionBase
{
    friend class WireGuardConnectionImpl;
    Q_OBJECT

public:
    WireGuardConnection(QObject *parent, Helper *helper, types::Protocol protocol, const WireGuardSessionParams &sessionParams);
    ~WireGuardConnection() override;

    void startConnect() override;
    void startDisconnect() override;
    bool isDisconnected() const override;

    //QString getConnectedTapTunAdapterName() override;

    static QString getWireGuardExeName();
    static QString getWireGuardAdapterName();

protected:
    void run() override;

private slots:
    void onProcessKillTimeout();

private:
    enum class ConnectionState { DISCONNECTED, CONNECTING, CONNECTED };
    static constexpr int PROCESS_KILL_TIMEOUT = 10000;

    ConnectionState getCurrentState() const;
    void setCurrentState(ConnectionState state);
    void setCurrentStateAndEmitSignal(ConnectionState state);
    void setError(CONNECT_ERROR err);
    bool checkForKernelModule();

    Helper *helper_;
    bool using_kernel_module_;
    std::unique_ptr<WireGuardConnectionImpl> pimpl_;
    ConnectionState current_state_;
    mutable QMutex current_state_mutex_;
    std::atomic<bool> do_stop_thread_;
    QTimer kill_process_timer_;
    AdapterGatewayInfo adapterGatewayInfo_;
};
