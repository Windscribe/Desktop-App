#ifndef IKEV2CONNECTION_MAC_H
#define IKEV2CONNECTION_MAC_H

#include <QObject>
#include <QTimer>
#include <QRecursiveMutex>
#include "iconnection.h"
#include "networkextensionlog_mac.h"
#include "engine/helper/helper_mac.h"

class IKEv2Connection_mac : public IConnection
{
    Q_OBJECT
public:
    explicit IKEv2Connection_mac(QObject *parent, IHelper *helper);
    ~IKEv2Connection_mac() override;

    // config path for openvpn, url for ikev2
    void startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName,
                      const QString &username, const QString &password, const ProxySettings &proxySettings,
                      const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode) override;
    void startDisconnect() override;
    bool isDisconnected() const override;
    ConnectionType getConnectionType() const override { return ConnectionType::IKEV2; }

    void continueWithUsernameAndPassword(const QString &username, const QString &password) override;
    void continueWithPassword(const QString &password) override;


    static void removeIkev2ConnectionFromOS();
    static void closeWindscribeActiveConnection();

private slots:
    void handleNotificationImpl(int status);
    void onStatisticsTimer();

private:
    enum {STATE_DISCONNECTED, STATE_START_CONNECT, STATE_START_DISCONNECTING, STATE_CONNECTED, STATE_DISCONNECTING_AUTH_ERROR, STATE_DISCONNECTING_ANY_ERROR};

    int state_;

    Helper_mac *helper_;
    bool bConnected_;
    mutable QRecursiveMutex mutex_;
    void *notificationId_;
    bool isStateConnectingAfterClick_;
    bool isDisconnectClicked_;
    NetworkExtensionLog_mac networkExtensionLog_;

    static constexpr int STATISTICS_UPDATE_PERIOD = 1000;
    QTimer statisticsTimer_;
    QString ipsecAdapterName_;

    int prevConnectionStatus_;
    bool isPrevConnectionStatusInitialized_;

    bool setKeyChain(const QString &username, const QString &password);
    void handleNotification(void *notification);
    bool isFailedAuthError(QMap<time_t, QString> &logs);
    bool isSocketError(QMap<time_t, QString> &logs);
};

#endif // IKEV2CONNECTION_MAC_H
