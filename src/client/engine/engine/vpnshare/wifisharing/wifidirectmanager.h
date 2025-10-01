#pragma once

#include <QMutex>
#include <QObject>
#include "types/enums.h"
#include "winrt_headers.h"

class WiFiDirectManager : public QObject
{
    Q_OBJECT
public:
    explicit WiFiDirectManager(QObject *parent);
    virtual ~WiFiDirectManager();

    bool isSupported();

    bool start(const QString &ssid, const QString &password);
    void stop();

    int getConnectedUsersCount();

signals:
    void started();
    void failed(WIFI_SHARING_ERROR error);
    void usersCountChanged();

private:
    winrt::Windows::Devices::WiFiDirect::WiFiDirectAdvertisementPublisher publisher_;
    winrt::Windows::Devices::WiFiDirect::WiFiDirectAdvertisement advertisement_;
    winrt::Windows::Devices::WiFiDirect::WiFiDirectLegacySettings legacySettings_;
    winrt::Windows::Devices::WiFiDirect::WiFiDirectConnectionListener connectionListener_;

    // Keep references to all connected peers
    std::vector<winrt::Windows::Devices::WiFiDirect::WiFiDirectDevice> connectedDevices_;
    QMutex mutex_;

    // Used to un-register for events
    winrt::event_token connectionRequestedToken_;
    winrt::event_token statusChangedToken_;

    // Clear out old state
    void reset();
    void startListener();
    void onStatusChanged(winrt::Windows::Devices::WiFiDirect::WiFiDirectAdvertisementPublisher const& sender,
                         winrt::Windows::Devices::WiFiDirect::WiFiDirectAdvertisementPublisherStatusChangedEventArgs const& args);
    void onConnectionRequested(winrt::Windows::Devices::WiFiDirect::WiFiDirectConnectionListener const& sender,
                               winrt::Windows::Devices::WiFiDirect::WiFiDirectConnectionRequestedEventArgs const& args);
};

