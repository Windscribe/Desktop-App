#include "wifidirectmanager.h"
#include "utils/logger.h"

#pragma comment(lib, "windowsapp")

WiFiDirectManager::WiFiDirectManager(QObject *parent) :
    QObject(parent),
    publisher_(nullptr),
    advertisement_(nullptr),
    legacySettings_(nullptr),
    connectionListener_(nullptr)
{
    winrt::init_apartment();
}

WiFiDirectManager::~WiFiDirectManager()
{
}

bool WiFiDirectManager::isSupported()
{
    return true;
}

bool WiFiDirectManager::start(const QString &ssid, const QString &password)
{
    // if the Mobile Hotspot feature is enabled in Windows, we must turn off it because it will conflict with our method
    try {
        auto connectionProfile { winrt::Windows::Networking::Connectivity::NetworkInformation::GetInternetConnectionProfile() };
        if (connectionProfile != nullptr) {
            auto tetheringManager = winrt::Windows::Networking::NetworkOperators::NetworkOperatorTetheringManager::CreateFromConnectionProfile(connectionProfile);
            if (tetheringManager != nullptr) {
                if (tetheringManager.TetheringOperationalState() != winrt::Windows::Networking::NetworkOperators::TetheringOperationalState::Off) {
                    auto ioAsync = tetheringManager.StopTetheringAsync();
                    auto fResult = ioAsync.get();
                }
            }
        }
    } catch (winrt::hresult_error const& ex) {
        winrt::hstring message = ex.message();
        qCDebug(LOG_WIFI_SHARED) << QString::fromStdWString(message.c_str());
    }

    reset();

    publisher_ = winrt::Windows::Devices::WiFiDirect::WiFiDirectAdvertisementPublisher();

    auto handler = [this](winrt::Windows::Devices::WiFiDirect::WiFiDirectAdvertisementPublisher const& sender,
                          winrt::Windows::Devices::WiFiDirect::WiFiDirectAdvertisementPublisherStatusChangedEventArgs const& args)
    {
        onStatusChanged(sender, args);
    };

    try {
        statusChangedToken_ = publisher_.StatusChanged(handler);

        // Set Advertisement required settings
        advertisement_ = publisher_.Advertisement();

        // Must set the autonomous group owner (GO) enabled flag
        // Legacy Wi-Fi Direct advertisement uses a Wi-Fi Direct GO to act as an access point to legacy settings
        advertisement_.IsAutonomousGroupOwnerEnabled(true);

        auto legacySettings = advertisement_.LegacySettings();

        // Must enable legacy settings so that non-Wi-Fi Direct peers can connect in legacy mode
        legacySettings.IsEnabled(true);
        // Set ssid and password
        std::wstring wsSsid = ssid.toStdWString();
        std::wstring wsPassword = password.toStdWString();
        auto credential = winrt::Windows::Security::Credentials::PasswordCredential();
        credential.Password(wsPassword);

        legacySettings.Ssid(wsSsid);
        legacySettings.Passphrase(credential);

        // Start the advertisement, which will create an access point that other peers can connect to
        publisher_.Start();
    } catch (winrt::hresult_error const& ex) {
        winrt::hstring message = ex.message();
        qCDebug(LOG_WIFI_SHARED) << QString::fromStdWString(message.c_str());
    }

    return true;
}

void WiFiDirectManager::stop()
{
    if (publisher_ == nullptr) {
        return;
    }
    // Call stop on the publisher and expect the status changed callback
    try {
        publisher_.Stop();
    }
    catch (winrt::hresult_error const& ex) {
        winrt::hstring message = ex.message();
        qCDebug(LOG_WIFI_SHARED) << QString::fromStdWString(message.c_str());
    }

    reset();
}

int WiFiDirectManager::getConnectedUsersCount()
{
    QMutexLocker locker(&mutex_);
    return connectedDevices_.size();
}

void WiFiDirectManager::reset()
{
    if (connectionListener_ != nullptr)
        connectionListener_.ConnectionRequested(connectionRequestedToken_);

    if (publisher_ != nullptr)
        publisher_.StatusChanged(statusChangedToken_);

    legacySettings_ = nullptr;
    advertisement_ = nullptr;
    publisher_ = nullptr;
    connectionListener_ = nullptr;
    connectedDevices_.clear();
}

void WiFiDirectManager::startListener()
{
    connectionListener_ = winrt::Windows::Devices::WiFiDirect::WiFiDirectConnectionListener();
    auto handler = [this](winrt::Windows::Devices::WiFiDirect::WiFiDirectConnectionListener const& sender, winrt::Windows::Devices::WiFiDirect::WiFiDirectConnectionRequestedEventArgs const& args)
    {
        onConnectionRequested(sender, args);
    };

    try {
        connectionRequestedToken_ = connectionListener_.ConnectionRequested(handler);
    } catch(winrt::hresult_error const& ex) {
        winrt::hstring message = ex.message();
        qCDebug(LOG_WIFI_SHARED) << QString::fromStdWString(message.c_str());
    }
}

void WiFiDirectManager::onStatusChanged(winrt::Windows::Devices::WiFiDirect::WiFiDirectAdvertisementPublisher const &sender, winrt::Windows::Devices::WiFiDirect::WiFiDirectAdvertisementPublisherStatusChangedEventArgs const &args)
{
    try {
        auto status = args.Status();

        switch (status) {
        case winrt::Windows::Devices::WiFiDirect::WiFiDirectAdvertisementPublisherStatus::Started:
        {
            startListener();
            qCDebug(LOG_WIFI_SHARED) << "Advertisement started";
            emit started();
            break;
        }
        case winrt::Windows::Devices::WiFiDirect::WiFiDirectAdvertisementPublisherStatus::Aborted:
        {
            auto error = args.Error();
            switch (error)
            {
                case winrt::Windows::Devices::WiFiDirect::WiFiDirectError::RadioNotAvailable:
                    qCDebug(LOG_WIFI_SHARED) << "Advertisement aborted, Wi-Fi radio is turned off";
                    break;

                case winrt::Windows::Devices::WiFiDirect::WiFiDirectError::ResourceInUse:
                    qCDebug(LOG_WIFI_SHARED) << "Advertisement aborted, Resource In Use";
                    break;

                default:
                    qCDebug(LOG_WIFI_SHARED) << "Advertisement aborted, unknown reason";
                    break;
            }
            break;
        }

        case winrt::Windows::Devices::WiFiDirect::WiFiDirectAdvertisementPublisherStatus::Stopped:
        {
            qCDebug(LOG_WIFI_SHARED) << "Advertisement stopped";
            break;
        }
        default:
            break;
        }

    } catch(winrt::hresult_error const& ex) {
        winrt::hstring message = ex.message();
        qCDebug(LOG_WIFI_SHARED) << QString::fromStdWString(message.c_str());
    }
}

void WiFiDirectManager::onConnectionRequested(const winrt::Windows::Devices::WiFiDirect::WiFiDirectConnectionListener &sender, const winrt::Windows::Devices::WiFiDirect::WiFiDirectConnectionRequestedEventArgs &args)
{
    try {
        auto connectionRequest = args.GetConnectionRequest();
        auto deviceInformation = connectionRequest.DeviceInformation();

        auto asyncAction = winrt::Windows::Devices::WiFiDirect::WiFiDirectDevice::FromIdAsync(deviceInformation.Id());


       auto handler = [this](winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Devices::WiFiDirect::WiFiDirectDevice> const &sender,
                              winrt::Windows::Foundation::AsyncStatus const asyncStatus)
       {
            if (asyncStatus == winrt::Windows::Foundation::AsyncStatus::Completed) {
                auto wifiDirectDevice = sender.GetResults();
                auto connectionStatus = wifiDirectDevice.ConnectionStatus();
                if (connectionStatus == winrt::Windows::Devices::WiFiDirect::WiFiDirectConnectionStatus::Connected) {
                    QMutexLocker locker(&mutex_);
                    connectedDevices_.push_back(wifiDirectDevice);
                    emit usersCountChanged();
                }

                auto connectionStatusChangedHandler = [this](winrt::Windows::Devices::WiFiDirect::WiFiDirectDevice const& sender,
                                                             winrt::Windows::Foundation::IInspectable const& object)
                {
                    if (sender.ConnectionStatus() == winrt::Windows::Devices::WiFiDirect::WiFiDirectConnectionStatus::Connected) {
                        QMutexLocker locker(&mutex_);
                        connectedDevices_.push_back(sender);
                        emit usersCountChanged();
                    } else {
                        QMutexLocker locker(&mutex_);
                        connectedDevices_.erase(std::remove(connectedDevices_.begin(), connectedDevices_.end(), sender), connectedDevices_.end());
                        emit usersCountChanged();
                    }
                };

                wifiDirectDevice.ConnectionStatusChanged(connectionStatusChangedHandler);
            }
       };

       asyncAction.Completed(handler);

    } catch(winrt::hresult_error const& ex) {
        winrt::hstring message = ex.message();
        qCDebug(LOG_WIFI_SHARED) << QString::fromStdWString(message.c_str());
    }
}
