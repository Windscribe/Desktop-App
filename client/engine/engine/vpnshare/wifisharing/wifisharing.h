#ifndef WIFISHARING_H
#define WIFISHARING_H

#include <QObject>
#include <QTimer>

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

private slots:
    void onWifiDirectStarted();

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

#endif // WIFISHARING_H
