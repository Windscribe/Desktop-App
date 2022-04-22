#ifndef WIREGUARDCONNECTION_WIN_H
#define WIREGUARDCONNECTION_WIN_H

#include <QScopedPointer>

#include <atomic>

#include "iconnection.h"
#include "engine/helper/helper_win.h"
#include "utils/servicecontrolmanager.h"
#include "wireguardringlogger.h"

class WireGuardConnection : public IConnection
{
    Q_OBJECT

public:
    WireGuardConnection(QObject *parent, IHelper *helper);
    ~WireGuardConnection() override;

    void startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName,
                      const QString &username, const QString &password, const ProxySettings &proxySettings,
                      const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode) override;
    void startDisconnect() override;
    bool isDisconnected() const override;

    ConnectionType getConnectionType() const override { return ConnectionType::WIREGUARD; }

    void continueWithUsernameAndPassword(const QString & /*username*/, const QString & /*password*/) override {}
    void continueWithPassword(const QString & /*password*/) override {}

    static QString getWireGuardExeName();
    static QString getWireGuardAdapterName();

protected:
    void run() override;

private Q_SLOTS:
    void onCheckServiceRunning();
    void onGetWireguardLogUpdates();
    void onGetWireguardStats();

private:
    Helper_win* const helper_;
    const WireGuardConfig* wireGuardConfig_ = nullptr;
    wsl::ServiceControlManager serviceCtrlManager_;
    QScopedPointer< wsl::WireguardRingLogger > wireguardLog_;
    bool connectedSignalEmited_ = false;
    std::atomic<bool> stopRequested_;

private:
    void onWireguardServiceStartupFailure() const;
};

#endif // WIREGUARDCONNECTION_WIN_H
