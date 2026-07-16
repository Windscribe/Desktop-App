#include "vpnsharecontroller.h"

#include <QElapsedTimer>
#include <QHostAddress>
#include <QSettings>

#include "engine/connectionmanager/connectors/openvpn/availableport.h"
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "proxyauthconfig.h"
#include "utils/log/categories.h"
#include "utils/network_utils/network_utils.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"

VpnShareController::VpnShareController(QObject *parent, Helper *helper, INetworkDetectionManager *networkDetectionManager) : QObject(parent),
    networkDetectionManager_(networkDetectionManager),
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

void VpnShareController::startProxySharing(const types::ShareProxyGateway &settings)
{
    QMutexLocker locker(&mutex_);

    // Set this before any short-circuits so onNetworkChange can retry even if the initial bind attempt is deferred
    // (whileConnected without VPN) or failed (NO_LOCAL_IP at startup, before any interface has an address).
    wantSharing_ = true;

    const bool sameTypeRunning =
        (settings.proxySharingMode == PROXY_SHARING_HTTP && httpProxyServer_) ||
        (settings.proxySharingMode == PROXY_SHARING_SOCKS && socksProxyServer_);

    if (sameTypeRunning && lastSettings_.functionallyEqual(settings)) {
        // Nothing that affects the running server changed; just record the new settings (e.g. transient flag clear).
        lastSettings_ = settings;
        return;
    }

    const uint prevSettingsPort = lastSettings_.port;
    lastSettings_ = settings;
    type_ = settings.proxySharingMode;
    // Set the port if provided, otherwise find a free port.  We do this here because we want to give a port back
    // to the caller in getProxySharingAddress(), even if the proxy sharing is not started immediately, so that
    // the user can set up their clients.  It is possible that when we actually start the proxy , we won't be able
    // to bind to this port, in which case we'll try to try again, but in most cases the first choice will work.
    //
    // On a rebind (network change, VPN cycle), settings.port hasn't changed but port_ may already hold the fallback
    // we picked on a prior call. Re-running findPort would just go through the same user-port-busy → fallback dance
    // every rebind, repeating the warning log. Only re-resolve when we're actually starting fresh or the user changed
    // their port preference.
    if (port_ == 0 || settings.port != prevSettingsPort) {
        port_ = findPort(settings.port);
    }
    shareWhileConnected_ = settings.whileConnected;

    // Tear down anything currently running so we can apply the new settings.
    SAFE_DELETE(httpProxyServer_);
    SAFE_DELETE(socksProxyServer_);

    if (shareWhileConnected_ && !vpnConnected_) {
        // Don't bind until the VPN comes up; onConnectedToVPNEvent will re-call us.
        return;
    }

    // Bind to the IPv4 address of the engine's known primary interface. If the engine doesn't have one yet (no network)
    // or the interface has no IPv4, there's nothing reachable to bind to; surface the error.
    QHostAddress bindAddr;
    int bindPrefix = 0;
    if (networkDetectionManager_) {
        types::NetworkInterface iface;
        networkDetectionManager_->getCurrentNetworkInterface(iface);
        NetworkUtils::getInterfaceAddress(iface, bindAddr, bindPrefix);
    }
    if (bindAddr.isNull()) {
        qCWarning(LOG_BASIC) << "Proxy sharing not started: no local IPv4 to bind to";
        bindIp_ = QHostAddress();
        bindPrefix_ = 0;
        emit proxySharingFailed(PROXY_SHARING_ERROR_NO_LOCAL_IP);
        return;
    }

    ProxyAuth::Config auth;
    auth.required = settings.requireAuth;
    auth.username = settings.username;
    auth.password = settings.password;

    // Refuse to start an auth-required proxy with empty credentials. ProxyAuth::secureEqual("", "") is true, so a
    // client sending empty creds would otherwise authenticate, leaving the proxy effectively open.
    if (auth.required && (auth.username.isEmpty() || auth.password.isEmpty())) {
        qCWarning(LOG_BASIC) << "Proxy sharing not started: requireAuth is set but credentials are empty";
        bindIp_ = QHostAddress();
        bindPrefix_ = 0;
        emit proxySharingFailed(PROXY_SHARING_ERROR_MISSING_CREDENTIALS);
        return;
    }

    // Start the proxy sharing
    bool isStarted = false;
    if (type_ == PROXY_SHARING_HTTP) {
        httpProxyServer_ = new HttpProxyServer::HttpProxyServer(this);
        connect(httpProxyServer_, &HttpProxyServer::HttpProxyServer::usersCountChanged, this, &VpnShareController::onProxyUsersCountChanged);
        isStarted = httpProxyServer_->startServer(bindAddr, bindPrefix, port_, auth);
        if (!isStarted) {
            // Port became busy, try to find another one
            port_ = findPort(18888);
            isStarted = httpProxyServer_->startServer(bindAddr, bindPrefix, port_, auth);
        }
    } else if (type_ == PROXY_SHARING_SOCKS) {
        socksProxyServer_ = new SocksProxyServer::SocksProxyServer(this);
        connect(socksProxyServer_, &SocksProxyServer::SocksProxyServer::usersCountChanged, this, &VpnShareController::onProxyUsersCountChanged);
        isStarted = socksProxyServer_->startServer(bindAddr, bindPrefix, port_, auth);
        if (!isStarted) {
            // Port became busy, try to find another one
            port_ = findPort(18888);
            isStarted = socksProxyServer_->startServer(bindAddr, bindPrefix, port_, auth);
        }
    } else {
        WS_ASSERT(false);
    }

    if (isStarted) {
        // Cache only after a successful bind so getProxySharingAddress() and onNetworkChange() reflect what's
        // actually listening, not what we tried to bind to.
        bindIp_ = bindAddr;
        bindPrefix_ = bindPrefix;
        // Notify the GUI about the new bind address. Engine::startProxySharingImpl emits its own state-change for
        // user-toggle starts, but rebind paths (onNetworkChange, onConnectedToVPNEvent) call this method directly,
        // so the address would otherwise stay stuck on whatever the GUI last saw.
        emit connectedProxyUsersChanged(true, type_, getProxySharingAddress(), 0);
    } else {
        qCWarning(LOG_BASIC) << "Proxy sharing not started: port" << port_ << "is busy";
        SAFE_DELETE(httpProxyServer_);
        SAFE_DELETE(socksProxyServer_);
        bindIp_ = QHostAddress();
        bindPrefix_ = 0;
        emit proxySharingFailed(PROXY_SHARING_ERROR_PORT_BUSY);
    }
}

void VpnShareController::userStopProxySharing()
{
    QMutexLocker locker(&mutex_);
    // Wipe everything that could later trigger an auto-restart (whileConnected on next VPN reconnect,
    // lastSettings_ caching old credentials, etc.).
    wantSharing_ = false;
    shareWhileConnected_ = false;
    lastSettings_ = {};
    tearDownServers();
}

void VpnShareController::tearDownServers()
{
    SAFE_DELETE(httpProxyServer_);
    SAFE_DELETE(socksProxyServer_);
    bindIp_ = QHostAddress();
    bindPrefix_ = 0;
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

    // Prefer the cached IP we are actually listening on. If nothing is currently bound (e.g. whileConnected is true
    // and the VPN isn't up yet), look up the engine's primary interface so the displayed address matches what we will
    // bind to once the proxy starts.
    QString ip = bindIp_.toString();
    if (bindIp_.isNull() && networkDetectionManager_) {
        types::NetworkInterface iface;
        networkDetectionManager_->getCurrentNetworkInterface(iface);
        QHostAddress addr;
        int prefix = 0;
        NetworkUtils::getInterfaceAddress(iface, addr, prefix);
        if (!addr.isNull()) {
            ip = addr.toString();
        }
    }

    if (httpProxyServer_) {
        return ip + ":" + QString::number(httpProxyServer_->serverPort());
    } else if (socksProxyServer_) {
        return ip + ":" + QString::number(socksProxyServer_->serverPort());
    }

    // Server not started yet, most likely because whileConnected is true and VPN is not connected
    return ip + ":" + QString::number(port_);
}

void VpnShareController::onWifiUsersCountChanged()
{
    QMutexLocker locker(&mutex_);
    int cntUsers = 0;
    #ifdef Q_OS_WIN
        if (wifiSharing_ && wifiSharing_->isSharingStarted()) {
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
    if (httpProxyServer_) {
        cntUsers += httpProxyServer_->getConnectedUsersCount();
        emit connectedProxyUsersChanged(isProxySharingEnabled(), PROXY_SHARING_HTTP, getProxySharingAddress(), cntUsers);
    } else if (socksProxyServer_) {
        cntUsers += socksProxyServer_->getConnectedUsersCount();
        emit connectedProxyUsersChanged(isProxySharingEnabled(), PROXY_SHARING_SOCKS, getProxySharingAddress(), cntUsers);
    }
}

void VpnShareController::startWifiSharing(const QString &ssid, const QString &password)
{
    QMutexLocker locker(&mutex_);
#ifdef Q_OS_WIN
    if (!wifiSharing_->isSharingStarted()) {
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
    if (wifiSharing_->isSharingStarted()) {
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

    if (wifiSharingStarted) {
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
    // wantSharing_ is the source of truth for "user wants the gateway running". Without that gate, a user who toggled
    // the gateway off while VPN was up would see it silently come back on the next VPN reconnect (whileConnected_ and
    // lastSettings_ would still carry the old enabled state).
    if (shareWhileConnected_ && wantSharing_) {
        startProxySharing(lastSettings_);
    }
}

void VpnShareController::onNetworkChange(const types::NetworkInterface &networkInterface)
{
    QMutexLocker locker(&mutex_);

    // Only react if the user has asked for proxy sharing. We deliberately don't gate on httpProxyServer_/socksProxyServer_
    // existing — if a previous bind attempt failed (e.g. NO_LOCAL_IP at startup before the network was up) we want this
    // event to be the trigger that finally binds.
    if (!wantSharing_) {
        return;
    }

    QHostAddress newIp;
    int newPrefix = 0;
    NetworkUtils::getInterfaceAddress(networkInterface, newIp, newPrefix);

    // Wi-Fi roaming, DHCP renewal, and similar churn briefly leave the interface with no IPv4. Don't tear down a
    // live binding for that — wait for the next event that brings a real address. Tell the GUI to display
    // "Unavailable" while we wait, so the address doesn't keep showing the now-stale IP.
    if (newIp.isNull()) {
        qCInfo(LOG_BASIC) << "Proxy sharing: network change with no local IP yet, deferring rebind";
        emit connectedProxyUsersChanged(true, type_, ":" + QString::number(port_), 0);
        return;
    }

    if (newIp == bindIp_ && newPrefix == bindPrefix_) {
        // No rebind needed, but we may have previously told the GUI "Unavailable" during a brief blip and the
        // same IP just came back. Re-emit the current address so the display recovers.
        emit connectedProxyUsersChanged(true, type_, getProxySharingAddress(), 0);
        return;
    }

    qCInfo(LOG_BASIC) << "Proxy sharing rebinding to" << newIp.toString() << "/" << newPrefix
                      << "(was" << bindIp_.toString() << "/" << bindPrefix_ << ")";

    // Tear down before re-entering startProxySharing so its same-settings short-circuit doesn't skip the rebind.
    SAFE_DELETE(httpProxyServer_);
    SAFE_DELETE(socksProxyServer_);
    startProxySharing(lastSettings_);
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
        // Pause, don't stop: tear the servers down but preserve user intent (wantSharing_, lastSettings_,
        // shareWhileConnected_) so onConnectedToVPNEvent can resume on reconnect.
        tearDownServers();
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
