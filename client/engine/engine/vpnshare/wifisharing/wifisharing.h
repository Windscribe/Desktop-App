#pragma once

#include <QObject>
#include <QTimer>

#include "types/enums.h"

class IHelper;
class WiFiDirectManager;
class IcsManager;

class WifiSharing : public QObject
{
    Q_OBJECT
public:
    explicit WifiSharing(QObject *parent, IHelper *helper);
    virtual ~WifiSharing();

    bool isSupported();

    void startSharing(const QString &ssid, const QString &password);
    void stopSharing();

    QString getSsid() { return ssid_; }

    bool isSharingStarted();

    void switchSharingForDisconnected();
    void switchSharingForConnected(const QString &vpnAdapterName);

    int getConnectedUsersCount();

signals:
    void usersCountChanged();
    void failed(WIFI_SHARING_ERROR error);

private slots:
    void onWifiDirectStarted();
    void onWifiDirectFailed(WIFI_SHARING_ERROR error);

private:
    bool isSharingStarted_;
    QString ssid_;
    WiFiDirectManager *wifiDirectManager_;
    IcsManager *icsManager_;
    QString vpnAdapterName_;

    enum {STATE_CONNECTING_CONNECTED, STATE_DISCONNECTED};
    int sharingState_;

    void updateICS(const QString &vpnAdapterName);
};
