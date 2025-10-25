#pragma once

#include "iconnection.h"
#include <atomic>
#include <QMutex>
#include <QTimer>
#include "engine/helper/helper.h"


class WireGuardConnection : public IConnection
{
    Q_OBJECT

public:
    WireGuardConnection(QObject *parent, Helper *helper);
    ~WireGuardConnection() override;

    void startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName,
                      const QString &username, const QString &password, const types::ProxySettings &proxySettings,
                      const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode,
                      bool isCustomConfig, const QString &overrideDnsIp) override;
    void startDisconnect() override;
    bool isDisconnected() const override;

    ConnectionType getConnectionType() const override { return ConnectionType::WIREGUARD; }

    void continueWithUsernameAndPassword(const QString & /*username*/, const QString & /*password*/) override {}
    void continueWithPassword(const QString & /*password*/) override {}

    static void stopWindscribeActiveConnection();

private slots:
    void handleNotificationImpl();
    void onStatisticsTimer();

private:
    enum {STATE_DISCONNECTED, STATE_START_CONNECT, STATE_START_DISCONNECTING, STATE_CONNECTED};
    int state_;
    Helper *helper_;
    mutable QRecursiveMutex mutex_;
    inline static const QString kVpnDescription = QStringLiteral("Windscribe WireGuard VPN");
    void *notificationId_;
    void *currentSession_;

    static constexpr int STATISTICS_UPDATE_PERIOD = 1000;
    QTimer statisticsTimer_;

    // True if startConnect() method was called and NEVPNManager emitted notification NEVPNStatusConnecting.
    // False otherwise.
    bool isConnectingStateReachedAfterStartingConnection_;

    QDateTime startConnect_;
    AdapterGatewayInfo adapterGatewayInfo_;

    void handleNotification(void *notification);
    QString connectionStatusToString(int status) const;
    void removeVPNConfigurationFromSystemPreferences();

};
