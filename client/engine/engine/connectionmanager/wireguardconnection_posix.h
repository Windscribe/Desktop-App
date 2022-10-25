#ifndef WIREGUARDCONNECTION_H
#define WIREGUARDCONNECTION_H

#include "iconnection.h"
#include <atomic>
#include <QMutex>
#include <QTimer>

class WireGuardConnectionImpl;

class WireGuardConnection : public IConnection
{
    friend class WireGuardConnectionImpl;
    Q_OBJECT

public:
    WireGuardConnection(QObject *parent, IHelper *helper);
    ~WireGuardConnection() override;

    void startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName,
                      const QString &username, const QString &password, const types::ProxySettings &proxySettings,
                      const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode) override;
    void startDisconnect() override;
    bool isDisconnected() const override;

    //QString getConnectedTapTunAdapterName() override;
    ConnectionType getConnectionType() const override { return ConnectionType::WIREGUARD; }

    void continueWithUsernameAndPassword(const QString & /*username*/, const QString & /*password*/) override {}
    void continueWithPassword(const QString & /*password*/) override {}

    static QString getWireGuardExeName();
    static QString getWireGuardAdapterName();

protected:
    void run() override;

private slots:
    void onProcessKillTimeout();

private:
    enum class ConnectionState { DISCONNECTED, CONNECTING, CONNECTED };
    static constexpr int PROCESS_KILL_TIMEOUT = 10000;
    static constexpr int kTimeoutForAutomatic = 20000;  // 20 secs timeout for the automatic connection mode

    ConnectionState getCurrentState() const;
    void setCurrentState(ConnectionState state);
    void setCurrentStateAndEmitSignal(ConnectionState state);
    void setError(CONNECT_ERROR err);
    bool checkForKernelModule();

    IHelper *helper_;
    bool using_kernel_module_;
    std::unique_ptr<WireGuardConnectionImpl> pimpl_;
    ConnectionState current_state_;
    mutable QMutex current_state_mutex_;
    std::atomic<bool> do_stop_thread_;
    QTimer kill_process_timer_;
    AdapterGatewayInfo adapterGatewayInfo_;
    bool isAutomaticConnectionMode_;
};

#endif // WIREGUARDCONNECTION_H
