#ifndef VPNSHARECONTROLLER_H
#define VPNSHARECONTROLLER_H

#include <QObject>
#include <QMutex>
#include "httpproxyserver/httpproxyserver.h"
#include "socksproxyserver/socksproxyserver.h"
#include "engine/helper/ihelper.h"
#include "types/enums.h"

#ifdef Q_OS_WIN
    #include "WifiSharing/wifisharing.h"
#endif

// thread safe
class VpnShareController : public QObject
{
    Q_OBJECT
public:
    explicit VpnShareController(QObject *parent, IHelper *helper);
    virtual ~VpnShareController();

    void onConnectingOrConnectedToVPNEvent(const QString &vpnAdapterName);
    void onDisconnectedFromVPNEvent();

    bool isUpdateIcsInProgress();

    void startProxySharing(PROXY_SHARING_TYPE proxyType);
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

private slots:
    void onWifiUsersCountChanged();
    void onProxyUsersCountChanged();

private:
    QRecursiveMutex mutex_;
    IHelper *helper_;
    HttpProxyServer::HttpProxyServer *httpProxyServer_;
    SocksProxyServer::SocksProxyServer *socksProxyServer_;
#ifdef Q_OS_WIN
    WifiSharing *wifiSharing_;
#endif

    bool getLastSavedPort(uint &outPort);
    void saveLastPort(uint port);
};

#endif // VPNSHARECONTROLLER_H
