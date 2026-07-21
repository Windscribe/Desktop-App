#pragma once

#include <QObject>
#include <QRecursiveMutex>
#include <QTimer>

#include "engine/connectionmanager/connectors/ikev2/ikev2connectionbase.h"
#include "engine/helper/helper.h"

class IKEv2Connection_mac : public Ikev2ConnectionBase
{
    Q_OBJECT
public:
    explicit IKEv2Connection_mac(QObject *parent, Helper *helper, types::Protocol protocol, const Ikev2SessionParams &sessionParams);
    ~IKEv2Connection_mac() override;

    void startConnect() override;
    void startDisconnect() override;
    bool isDisconnected() const override;

    static void removeIkev2ConnectionFromOS();
    static void closeAppActiveConnection();

private slots:
    void handleNotificationImpl(int status);
    void onStatisticsTimer();

private:
    enum {STATE_DISCONNECTED, STATE_START_CONNECT, STATE_START_DISCONNECTING, STATE_CONNECTED, STATE_DISCONNECTING_AUTH_ERROR, STATE_DISCONNECTING_ANY_ERROR};

    int state_;

    Helper *helper_;
    bool bConnected_;
    mutable QRecursiveMutex mutex_;
    void *notificationId_;
    bool isStateConnectingAfterClick_;
    bool isDisconnectClicked_;

    static constexpr int STATISTICS_UPDATE_PERIOD = 1000;
    QTimer statisticsTimer_;
    QString ipsecAdapterName_;

    int prevConnectionStatus_;
    bool isPrevConnectionStatusInitialized_;

    // True if startConnect() method was called and NEVPNManager emitted notification NEVPNStatusConnecting.
    // False otherwise.
    bool isConnectingStateReachedAfterStartingConnection_;

    QDateTime startConnect_;

    bool setKeyChain(const QString &username, const QString &password);
    void handleNotification(void *notification);
    bool isFailedAuthError(QMap<time_t, QString> &logs);
    bool isSocketError(QMap<time_t, QString> &logs);
    bool setCustomDns(const QString &overrideDnsIpAddress);
};
