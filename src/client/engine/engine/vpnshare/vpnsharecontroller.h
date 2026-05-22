#pragma once

#include <QHostAddress>
#include <QObject>
#include <QMutex>
#include "httpproxyserver/httpproxyserver.h"
#include "socksproxyserver/socksproxyserver.h"
#include "engine/helper/helper.h"
#include "types/enums.h"
#include "types/networkinterface.h"
#include "types/shareproxygateway.h"

#ifdef Q_OS_WIN
    #include "WifiSharing/wifisharing.h"
#endif

class INetworkDetectionManager;

// thread safe
class VpnShareController : public QObject
{
    Q_OBJECT
public:
    explicit VpnShareController(QObject *parent, Helper *helper, INetworkDetectionManager *networkDetectionManager);
    virtual ~VpnShareController();

    void onConnectedToVPNEvent(const QString &vpnAdapterName);
    void onDisconnectedFromVPNEvent();
    void onNetworkChange(const types::NetworkInterface &networkInterface);

    void startProxySharing(const types::ShareProxyGateway &settings);
    // User-explicit stop: tears down servers AND clears wantSharing_/shareWhileConnected_/lastSettings_ so a
    // subsequent VPN cycle or network event can't auto-restart. For internal pause (e.g. VPN disconnect with
    // shareWhileConnected), use tearDownServers() instead so user intent is preserved.
    void userStopProxySharing();

    bool isWifiSharingSupported();
    void startWifiSharing(const QString &ssid, const QString &password);
    void stopWifiSharing();

    bool isProxySharingEnabled();
    bool isWifiSharingEnabled();
    QString getCurrentCaption();
    QString getProxySharingAddress();

signals:
    void connectedWifiUsersChanged(bool bEnabled, const QString &ssid, int usersCount);
    void connectedProxyUsersChanged(bool bEnabled, PROXY_SHARING_TYPE type, const QString &address, int usersCount);
    void wifiSharingFailed(WIFI_SHARING_ERROR error);
    void proxySharingFailed(PROXY_SHARING_ERROR error);

private slots:
    void onWifiUsersCountChanged();
    void onProxyUsersCountChanged();

private:
    QRecursiveMutex mutex_;
    INetworkDetectionManager *networkDetectionManager_;
    bool vpnConnected_;
    bool shareWhileConnected_;
    // Tracks user intent independent of bind state. True between startProxySharing() and stopProxySharing(),
    // even if a bind attempt failed (e.g. NO_LOCAL_IP). onNetworkChange uses this to retry binding when a
    // usable IP eventually appears.
    bool wantSharing_ = false;
    PROXY_SHARING_TYPE type_;
    uint port_;
    QHostAddress bindIp_;
    int bindPrefix_ = 0;
    types::ShareProxyGateway lastSettings_;
    HttpProxyServer::HttpProxyServer *httpProxyServer_;
    SocksProxyServer::SocksProxyServer *socksProxyServer_;
#ifdef Q_OS_WIN
    WifiSharing *wifiSharing_;
#endif

    bool getLastSavedPort(uint &outPort);
    void saveLastPort(uint port);
    uint findPort(uint userPort);

    // Close any running proxy servers and clear the cached bind address. Caller must hold mutex_. Does NOT touch
    // wantSharing_/shareWhileConnected_/lastSettings_, so a VPN-cycle pause can resume on reconnect without the
    // user having to re-toggle.
    void tearDownServers();
};
