#include "vpnsharecontroller.h"

#include <QElapsedTimer>
#include <QSettings>

#include "engine/connectionmanager/availableport.h"
#include "utils/network_utils/network_utils.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"

VpnShareController::VpnShareController(QObject *parent, IHelper *helper) : QObject(parent),
    helper_(helper),
    httpProxyServer_(NULL),
    socksProxyServer_(NULL),
    bProxySharingActivated_(false)
#ifdef Q_OS_WIN
    ,wifiSharing_(NULL)
#endif
{
#ifdef Q_OS_WIN
    wifiSharing_ = new WifiSharing(this, helper_);
    connect(wifiSharing_, &WifiSharing::usersCountChanged, this, &VpnShareController::onWifiUsersCountChanged);
    connect(wifiSharing_, &WifiSharing::failed, this, &VpnShareController::wifiSharingFailed);
#endif
}

VpnShareController::~VpnShareController()
{
    SAFE_DELETE(httpProxyServer_);
    SAFE_DELETE(socksProxyServer_);
#ifdef Q_OS_WIN
    SAFE_DELETE(wifiSharing_);
#endif
}

void VpnShareController::activateProxySharing(PROXY_SHARING_TYPE proxyType, uint port)
{
    // don't actually start the service unless the VPN is on
    // it will be considered activated but not started till VPN is turned on
    bProxySharingActivated_ = true;
    ActivatedProxySharingType_ = proxyType;
    port_ = port;
}

void VpnShareController::startProxySharing(PROXY_SHARING_TYPE proxyType, uint port)
{
    QMutexLocker locker(&mutex_);

    SAFE_DELETE(httpProxyServer_);
    SAFE_DELETE(socksProxyServer_);

    if(!bProxySharingActivated_)
    {
        return;
    }

    if (proxyType == PROXY_SHARING_HTTP)
    {
        httpProxyServer_ = new HttpProxyServer::HttpProxyServer(this);
        connect(httpProxyServer_, &HttpProxyServer::HttpProxyServer::usersCountChanged, this, &VpnShareController::onProxyUsersCountChanged);

        bool isStarted = false;
        if (port != 0) {
            // Port provided by user, use it.
            isStarted = httpProxyServer_->startServer(port);
        } else {
            if (getLastSavedPort(port)) {
                isStarted = httpProxyServer_->startServer(port);
            }
            if (!isStarted) {
                uint randomPort = AvailablePort::getAvailablePort(18888);
                httpProxyServer_->startServer(randomPort);
                saveLastPort(randomPort);
            }
        }
    } else if (proxyType == PROXY_SHARING_SOCKS) {
        socksProxyServer_ = new SocksProxyServer::SocksProxyServer(this);
        connect(socksProxyServer_, &SocksProxyServer::SocksProxyServer::usersCountChanged, this, &VpnShareController::onProxyUsersCountChanged);

        bool isStarted = false;
        if (port != 0) {
            isStarted = socksProxyServer_->startServer(port);
        } else {
            if (getLastSavedPort(port)) {
                isStarted = socksProxyServer_->startServer(port);
            }
            if (!isStarted) {
                uint randomPort = AvailablePort::getAvailablePort(18888);
                socksProxyServer_->startServer(randomPort);
                saveLastPort(randomPort);
            }
        }
    } else {
        WS_ASSERT(false);
    }
}

void VpnShareController::deactivateProxySharing()
{
    bProxySharingActivated_ = false;
    stopProxySharing();
}

void VpnShareController::stopProxySharing()
{
    QMutexLocker locker(&mutex_);
    SAFE_DELETE_LATER(httpProxyServer_);
    SAFE_DELETE_LATER(socksProxyServer_);
}

bool VpnShareController::isProxySharingEnabled()
{
    QMutexLocker locker(&mutex_);
    return httpProxyServer_ != NULL || socksProxyServer_ != NULL;
}

bool VpnShareController::isWifiSharingEnabled()
{
    QMutexLocker locker(&mutex_);
#ifdef Q_OS_WIN
    return wifiSharing_->isSharingStarted();
#else
    return false;
#endif
}

QString VpnShareController::getProxySharingAddress()
{
    QMutexLocker locker(&mutex_);
    if (httpProxyServer_)
    {
        return NetworkUtils::getLocalIP() + ":" + QString::number(httpProxyServer_->serverPort());
    }
    else if (socksProxyServer_)
    {
        return NetworkUtils::getLocalIP() + ":" + QString::number(socksProxyServer_->serverPort());
    }
    WS_ASSERT(false);
    return "Unknown";
}

void VpnShareController::onWifiUsersCountChanged()
{
    QMutexLocker locker(&mutex_);
    int cntUsers = 0;
    #ifdef Q_OS_WIN
        if (wifiSharing_ && wifiSharing_->isSharingStarted())
        {
            cntUsers += wifiSharing_->getConnectedUsersCount();
        }
        emit connectedWifiUsersChanged(isWifiSharingEnabled(), wifiSharing_->getSsid(), cntUsers);
    #else
        emit connectedWifiUsersChanged(isWifiSharingEnabled(), "", cntUsers);
    #endif
}

void VpnShareController::onProxyUsersCountChanged()
{
    QMutexLocker locker(&mutex_);

    int cntUsers = 0;
    if (httpProxyServer_)
    {
        cntUsers += httpProxyServer_->getConnectedUsersCount();
        emit connectedProxyUsersChanged(isProxySharingEnabled(), PROXY_SHARING_HTTP, getProxySharingAddress(), cntUsers);
    }
    else if (socksProxyServer_)
    {
        cntUsers += socksProxyServer_->getConnectedUsersCount();
        emit connectedProxyUsersChanged(isProxySharingEnabled(), PROXY_SHARING_SOCKS, getProxySharingAddress(), cntUsers);
    }
}

void VpnShareController::startWifiSharing(const QString &ssid, const QString &password)
{
    QMutexLocker locker(&mutex_);
#ifdef Q_OS_WIN
    if (!wifiSharing_->isSharingStarted())
    {
        wifiSharing_->startSharing(ssid, password);
    }
#else
    Q_UNUSED(ssid);
    Q_UNUSED(password);
#endif
}

void VpnShareController::stopWifiSharing()
{
    QMutexLocker locker(&mutex_);
#ifdef Q_OS_WIN
    if (wifiSharing_->isSharingStarted())
    {
        wifiSharing_->stopSharing();
    }
#endif
}

QString VpnShareController::getCurrentCaption()
{
    QMutexLocker locker(&mutex_);

    bool wifiSharingStarted = false;
#ifdef Q_OS_WIN
    wifiSharingStarted = wifiSharing_->isSharingStarted();
#endif

    if (wifiSharingStarted)
    {
        QString s;
#ifdef Q_OS_WIN
        s = wifiSharing_->getSsid();
#endif

        if (httpProxyServer_)
        {
            s += " + HTTP";
        }
        else if (socksProxyServer_)
        {
            s += " + SOCKS";
        }

        return s;
    }
    else
    {
        if (httpProxyServer_)
        {
            return "HTTP";
        }
        else if (socksProxyServer_)
        {
            return "SOCKS";
        }
    }
    return "";
}

bool VpnShareController::isWifiSharingSupported()
{
    QMutexLocker locker(&mutex_);
#ifdef Q_OS_WIN
    return wifiSharing_->isSupported();
#else
    return false;
#endif
}

void VpnShareController::onConnectedToVPNEvent(const QString &vpnAdapterName)
{
    QMutexLocker locker(&mutex_);
#ifdef Q_OS_WIN
    if (wifiSharing_)
    {
        wifiSharing_->switchSharingForConnected(vpnAdapterName);
    }
#else
    Q_UNUSED(vpnAdapterName);
#endif
    if (httpProxyServer_)
    {
        httpProxyServer_->closeActiveConnections();
    }
    if (socksProxyServer_)
    {
        socksProxyServer_->closeActiveConnections();
    }
    if (bProxySharingActivated_)
    {
        startProxySharing(ActivatedProxySharingType_, port_);
    }
}

void VpnShareController::onDisconnectedFromVPNEvent()
{
    QMutexLocker locker(&mutex_);
#ifdef Q_OS_WIN
    if (wifiSharing_)
    {
        wifiSharing_->switchSharingForDisconnected();
    }
#endif
    if (httpProxyServer_)
    {
        httpProxyServer_->closeActiveConnections();
    }
    if (socksProxyServer_)
    {
        socksProxyServer_->closeActiveConnections();
    }
    stopProxySharing();
}

bool VpnShareController::getLastSavedPort(uint &outPort)
{
    QSettings settings;
    if (settings.contains("httpSocksProxyPort"))
    {
        bool bOk;
        uint port = settings.value("httpSocksProxyPort").toUInt(&bOk);
        if (bOk)
        {
            outPort = port;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

void VpnShareController::saveLastPort(uint port)
{
    QSettings settings;
    settings.setValue("httpSocksProxyPort", port);
}

