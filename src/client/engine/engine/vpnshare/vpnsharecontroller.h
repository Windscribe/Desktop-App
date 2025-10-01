#pragma once

#include <QObject>
#include <QMutex>
#include "httpproxyserver/httpproxyserver.h"
#include "socksproxyserver/socksproxyserver.h"
#include "engine/helper/helper.h"
#include "types/enums.h"

#ifdef Q_OS_WIN
    #include "WifiSharing/wifisharing.h"
#endif

// thread safe
class VpnShareController : public QObject
{
    Q_OBJECT
public:
    explicit VpnShareController(QObject *parent, Helper *helper);
    virtual ~VpnShareController();

    void onConnectedToVPNEvent(const QString &vpnAdapterName);
    void onDisconnectedFromVPNEvent();

    void startProxySharing(PROXY_SHARING_TYPE proxyType, uint port, bool whileConnected);
    void stopProxySharing();

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

private slots:
    void onWifiUsersCountChanged();
    void onProxyUsersCountChanged();

private:
    QRecursiveMutex mutex_;
    bool vpnConnected_;
    bool shareWhileConnected_;
    PROXY_SHARING_TYPE type_;
    uint port_;
    HttpProxyServer::HttpProxyServer *httpProxyServer_;
    SocksProxyServer::SocksProxyServer *socksProxyServer_;
#ifdef Q_OS_WIN
    WifiSharing *wifiSharing_;
#endif

    bool getLastSavedPort(uint &outPort);
    void saveLastPort(uint port);
    uint findPort(uint userPort);
};
