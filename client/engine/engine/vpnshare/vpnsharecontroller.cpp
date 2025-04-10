#include "vpnsharecontroller.h"

#include <QElapsedTimer>
#include <QSettings>

#include "engine/connectionmanager/availableport.h"
#include "utils/network_utils/network_utils.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"

VpnShareController::VpnShareController(QObject *parent, Helper *helper) : QObject(parent),
    httpProxyServer_(NULL),
    socksProxyServer_(NULL),
    vpnConnected_(false),
    shareWhileConnected_(false)
#ifdef Q_OS_WIN
    ,wifiSharing_(NULL)
#endif
{
#ifdef Q_OS_WIN
    wifiSharing_ = new WifiSharing(this, helper);
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

void VpnShareController::startProxySharing(PROXY_SHARING_TYPE proxyType, uint port, bool whileConnected)
{
    QMutexLocker locker(&mutex_);

    type_ = proxyType;
    // Set the port if provided, otherwise find a free port.  We do this here because we want to give a port back
    // to the caller in getProxySharingAddress(), even if the proxy sharing is not started immediately, so that
    // the user can set up their clients.  It is possible that when we actually start the proxy , we won't be able
    // to bind to this port, in which case we'll try to try again, but in most cases the first choice will work.
    port_ = findPort(port);
    shareWhileConnected_ = whileConnected;

    if (shareWhileConnected_ && !vpnConnected_) {
        // Share while connected is on, but VPN is not connected, so stop any existing proxy sharing
        if ((proxyType == PROXY_SHARING_HTTP && httpProxyServer_) || (proxyType == PROXY_SHARING_SOCKS && socksProxyServer_)) {
            SAFE_DELETE(httpProxyServer_);
            SAFE_DELETE(socksProxyServer_);
        }
        return;
    }

    // If already started, do nothing.
    if ((proxyType == PROXY_SHARING_HTTP && httpProxyServer_) || (proxyType == PROXY_SHARING_SOCKS && socksProxyServer_)) {
        // Already started
        return;
    } else {
        stopProxySharing();
        SAFE_DELETE(httpProxyServer_);
        SAFE_DELETE(socksProxyServer_);
    }

    // Start the proxy sharing
    if (proxyType == PROXY_SHARING_HTTP) {
        httpProxyServer_ = new HttpProxyServer::HttpProxyServer(this);
        connect(httpProxyServer_, &HttpProxyServer::HttpProxyServer::usersCountChanged, this, &VpnShareController::onProxyUsersCountChanged);
        bool isStarted = httpProxyServer_->startServer(port_);
        if (!isStarted) {
            // Port became busy, try to find another one
            port_ = findPort(18888);
            httpProxyServer_->startServer(port_);
        }
    } else if (proxyType == PROXY_SHARING_SOCKS) {
        socksProxyServer_ = new SocksProxyServer::SocksProxyServer(this);
        connect(socksProxyServer_, &SocksProxyServer::SocksProxyServer::usersCountChanged, this, &VpnShareController::onProxyUsersCountChanged);
        bool isStarted = socksProxyServer_->startServer(port_);
        if (!isStarted) {
            // Port became busy, try to find another one
            port_ = findPort(18888);
            socksProxyServer_->startServer(port_);
        }
    } else {
        WS_ASSERT(false);
    }
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
    if (httpProxyServer_) {
        return NetworkUtils::getLocalIP() + ":" + QString::number(httpProxyServer_->serverPort());
    } else if (socksProxyServer_) {
        return NetworkUtils::getLocalIP() + ":" + QString::number(socksProxyServer_->serverPort());
    }

    // Server not started yet, most likely because whileConnected is true and VPN is not connected
    return NetworkUtils::getLocalIP() + ":" + QString::number(port_);
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

        if (httpProxyServer_) {
            s += " + HTTP";
        } else if (socksProxyServer_) {
            s += " + SOCKS";
        }

        return s;
    } else {
        if (httpProxyServer_) {
            return "HTTP";
        } else if (socksProxyServer_) {
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
    if (wifiSharing_) {
        wifiSharing_->switchSharingForConnected(vpnAdapterName);
    }
#else
    Q_UNUSED(vpnAdapterName);
#endif
    if (httpProxyServer_) {
        httpProxyServer_->closeActiveConnections();
    }
    if (socksProxyServer_) {
        socksProxyServer_->closeActiveConnections();
    }

    vpnConnected_ = true;
    if (shareWhileConnected_) {
        startProxySharing(type_, port_, shareWhileConnected_);
    }
}

void VpnShareController::onDisconnectedFromVPNEvent()
{
    QMutexLocker locker(&mutex_);
#ifdef Q_OS_WIN
    if (wifiSharing_) {
        wifiSharing_->switchSharingForDisconnected();
    }
#endif
    if (httpProxyServer_) {
        httpProxyServer_->closeActiveConnections();
    }
    if (socksProxyServer_) {
        socksProxyServer_->closeActiveConnections();
    }

    vpnConnected_ = false;
    if (shareWhileConnected_) {
        stopProxySharing();
    }
}

bool VpnShareController::getLastSavedPort(uint &outPort)
{
    QSettings settings;
    if (settings.contains("httpSocksProxyPort")) {
        bool bOk;
        uint port = settings.value("httpSocksProxyPort").toUInt(&bOk);
        if (bOk) {
            outPort = port;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void VpnShareController::saveLastPort(uint port)
{
    QSettings settings;
    settings.setValue("httpSocksProxyPort", port);
}

uint VpnShareController::findPort(uint userPort)
{
    if (userPort != 0) {
        saveLastPort(userPort);
        return userPort;
    }

    uint port = 0;
    if (!getLastSavedPort(port)) {
        port = AvailablePort::getAvailablePort(18888);
        saveLastPort(port);
    }
    return port;
}
