#ifndef WIFISHARING_H
#define WIFISHARING_H

#include <QObject>
#include <QTimer>

class IHelper;
class WlanManager;
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
    void switchSharingForConnectingOrConnected(const QString &vpnAdapterName);

    int getConnectedUsersCount();

    bool isUpdateIcsInProgress();

signals:
    void usersCountChanged();

private slots:
    void onWlanStarted();
    void onTimerForUpdateIcsDisconnected();

private:
    bool isSharingStarted_;
    QString ssid_;
    WlanManager *wlanManager_;
    IcsManager *icsManager_;
    QString vpnAdapterName_;

    QTimer timerForUpdateIcsDisconnected_;
    static constexpr int DELAY_FOR_UPDATE_ICS_DISCONNECTED = 10000;

    enum {STATE_CONNECTING_CONNECTED, STATE_DISCONNECTED};
    int sharingState_;



    void updateICS(int state, const QString &vpnAdapterName);
};

#endif // WIFISHARING_H
