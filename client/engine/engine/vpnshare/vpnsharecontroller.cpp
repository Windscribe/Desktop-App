#include "vpnsharecontroller.h"
#include "utils/utils.h"
#include <QElapsedTimer>
#include "engine/connectionmanager/availableport.h"
#include <QSettings>

VpnShareController::VpnShareController(QObject *parent, IHelper *helper) : QObject(parent),
    mutex_(QRecursiveMutex()),
    helper_(helper),
    httpProxyServer_(NULL),
    socksProxyServer_(NULL)
#ifdef Q_OS_WIN
    ,wifiSharing_(NULL)
#endif
{
#ifdef Q_OS_WIN
    wifiSharing_ = new WifiSharing(this, helper_);
    connect(wifiSharing_, SIGNAL(usersCountChanged()), SLOT(onWifiUsersCountChanged()));
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

void VpnShareController::startProxySharing(PROXY_SHARING_TYPE proxyType)
{
    QMutexLocker locker(&mutex_);

    SAFE_DELETE(httpProxyServer_);
    SAFE_DELETE(socksProxyServer_);
    if (proxyType == PROXY_SHARING_HTTP)
    {
        httpProxyServer_ = new HttpProxyServer::HttpProxyServer(this);
        connect(httpProxyServer_, SIGNAL(usersCountChanged()), SLOT(onProxyUsersCountChanged()));

        uint port;
        bool isStarted = false;
        if (getLastSavedPort(port))
        {
            isStarted = httpProxyServer_->startServer(port);
        }
        if (!isStarted)
        {
            uint randomPort = AvailablePort::getAvailablePort(18888);
            httpProxyServer_->startServer(randomPort);
            saveLastPort(randomPort);
        }
    }
    else if (proxyType == PROXY_SHARING_SOCKS)
    {
        socksProxyServer_ = new SocksProxyServer::SocksProxyServer(this);
        connect(socksProxyServer_, SIGNAL(usersCountChanged()), SLOT(onProxyUsersCountChanged()));

        uint port;
        bool isStarted = false;
        if (getLastSavedPort(port))
        {
            isStarted = socksProxyServer_->startServer(port);
        }
        if (!isStarted)
        {
            uint randomPort = AvailablePort::getAvailablePort(18888);
            socksProxyServer_->startServer(randomPort);
            saveLastPort(randomPort);
        }
    }
    else
    {
        Q_ASSERT(false);
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
#elif defined Q_OS_MAC
    return false;
#else
    return false;
#endif
}

QString VpnShareController::getProxySharingAddress()
{
    QMutexLocker locker(&mutex_);
    if (httpProxyServer_)
    {
        return Utils::getLocalIP() + ":" + QString::number(httpProxyServer_->serverPort());
    }
    else if (socksProxyServer_)
    {
        return Utils::getLocalIP() + ":" + QString::number(socksProxyServer_->serverPort());
    }
    Q_ASSERT(false);
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
    #endif
        Q_EMIT connectedWifiUsersChanged(cntUsers);
}

void VpnShareController::onProxyUsersCountChanged()
{
    QMutexLocker locker(&mutex_);

    int cntUsers = 0;
    if (httpProxyServer_)
    {
        cntUsers += httpProxyServer_->getConnectedUsersCount();
    }
    else if (socksProxyServer_)
    {
        cntUsers += socksProxyServer_->getConnectedUsersCount();
    }
    Q_EMIT connectedProxyUsersChanged(cntUsers);
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
#elif defined Q_OS_MAC
    return false;
#else
    return false;
#endif
}

void VpnShareController::onConnectingOrConnectedToVPNEvent(const QString &vpnAdapterName)
{
    QMutexLocker locker(&mutex_);
#ifdef Q_OS_WIN
    if (wifiSharing_)
    {
        wifiSharing_->switchSharingForConnectingOrConnected(vpnAdapterName);
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
}

bool VpnShareController::isUpdateIcsInProgress()
{
#ifdef Q_OS_WIN
    if (wifiSharing_)
    {
        return wifiSharing_->isUpdateIcsInProgress();
    }
#endif
    return false;
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

