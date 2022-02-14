#include "wireguardconnection.h"
#include "utils/crashhandler.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/helper/ihelper.h"
#include "engine/types/types.h"
#include "engine/types/wireguardconfig.h"
#include "engine/types/wireguardtypes.h"

#ifdef Q_OS_WIN
    #include "adapterutils_win.h"
    #include "engine/helper/helper_win.h"
#elif defined (Q_OS_MAC) || defined (Q_OS_LINUX)
    #include "engine/helper/helper_posix.h"
#endif

class WireGuardConnectionImpl
{
public:
    explicit WireGuardConnectionImpl(WireGuardConnection *host);
    void setConfig(const WireGuardConfig *wireGuardConfig);
    void connect();
    void configure();
    void disconnect();
    bool getStatus(WireGuardStatus *status);
    bool stopWireGuard();

    QString getAdapterName() const { return adapterName_; }
 
private:
    WireGuardConnection *host_;
    QString adapterName_;
    WireGuardConfig config_;
    bool isDaemonRunning_;
};

WireGuardConnectionImpl::WireGuardConnectionImpl(WireGuardConnection *host)
    : host_(host),
      adapterName_(WireGuardConnection::getWireGuardAdapterName()),
      isDaemonRunning_(false)
{
}

void WireGuardConnectionImpl::setConfig(const WireGuardConfig *wireGuardConfig)
{
    if (wireGuardConfig)
        config_ = *wireGuardConfig;
}

void WireGuardConnectionImpl::connect()
{
    // Start daemon, if not running.
    if (!isDaemonRunning_) {
        int retry = 0;
        IHelper::ExecuteError err;
        while ((err = host_->helper_->startWireGuard(WireGuardConnection::getWireGuardExeName(), adapterName_)) != IHelper::EXECUTE_SUCCESS)
        {
            // don't bother another attempt if signature is invalid
            if (err == IHelper::EXECUTE_VERIFY_ERROR)
            {
                host_->setError(ProtoTypes::ConnectError::EXE_VERIFY_WIREGUARD_ERROR);
                return;
            }
            if (retry >= 2) {
                qCDebug(LOG_WIREGUARD) << "Can't start WireGuard daemon after" << retry << "retries";
                host_->setError(ProtoTypes::ConnectError::WIREGUARD_CONNECTION_ERROR);
                return;
            }
            ++retry;
            QThread::msleep(1000);
        }
        qCDebug(LOG_WIREGUARD) << "WireGuard daemon started after" << retry << "retries";
        isDaemonRunning_ = true;
    }
}

void WireGuardConnectionImpl::configure()
{
    Q_ASSERT(isDaemonRunning_);

    // Configure the client and the peer.
    if (!host_->helper_->configureWireGuard(config_)) {
        qCDebug(LOG_WIREGUARD) << "Failed to configure WireGuard daemon";
        host_->setError(ProtoTypes::ConnectError::WIREGUARD_CONNECTION_ERROR);
    }
}

void WireGuardConnectionImpl::disconnect()
{
    if(!stopWireGuard())
        return;
    host_->setCurrentStateAndEmitSignal(WireGuardConnection::ConnectionState::DISCONNECTED);
}

bool WireGuardConnectionImpl::getStatus(WireGuardStatus *status)
{
    return isDaemonRunning_ && host_->helper_->getWireGuardStatus(status);
}

bool WireGuardConnectionImpl::stopWireGuard()
{
    if (isDaemonRunning_) {
        int retry = 0;
        while (!host_->helper_->stopWireGuard()) {
            if (retry >= 4) {
                qCDebug(LOG_WIREGUARD) << "Can't stop WireGuard daemon after" << retry << "retries";
                return false;
            }
            ++retry;
            QThread::msleep(500);
        }
        qCDebug(LOG_WIREGUARD) << "WireGuard daemon stopped after" << retry << "retries";
        isDaemonRunning_ = false;
    }
    return true;
}

WireGuardConnection::WireGuardConnection(QObject *parent, IHelper *helper)
    : IConnection(parent),
      helper_(helper),
      pimpl_(new WireGuardConnectionImpl(this)),
      current_state_(ConnectionState::DISCONNECTED),
      do_stop_thread_(false)
{
    connect(&kill_process_timer_, SIGNAL(timeout()), SLOT(onProcessKillTimeout()));
}

WireGuardConnection::~WireGuardConnection()
{
    wait();
}

void WireGuardConnection::startConnect(const QString &configPathOrUrl, const QString &ip,
                                       const QString &dnsHostName, const QString &username,
                                       const QString &password, const ProxySettings &proxySettings,
                                       const WireGuardConfig *wireGuardConfig,
                                       bool isEnableIkev2Compression, bool isAutomaticConnectionMode)
{
    Q_UNUSED(configPathOrUrl);
    Q_UNUSED(ip);
    Q_UNUSED(dnsHostName);
    Q_UNUSED(username);
    Q_UNUSED(password);
    Q_UNUSED(proxySettings);
    Q_UNUSED(isEnableIkev2Compression);
    Q_UNUSED(isAutomaticConnectionMode);

    qCDebug(LOG_CONNECTION) << "Connecting WireGuard:" << pimpl_->getAdapterName();

    do_stop_thread_ = true;
    wait();
    do_stop_thread_ = false;

    pimpl_->setConfig(wireGuardConfig);

    // for windows adapterGatewayInfo_ filled on emit connected below
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
    // note: route gateway not used for WireGuard in AdapterGatewayInfo
    adapterGatewayInfo_.clear();
    adapterGatewayInfo_.setAdapterName(pimpl_->getAdapterName());
    QStringList address_and_cidr = wireGuardConfig->clientIpAddress().split('/');
    if (address_and_cidr.size() > 1)
    {
        adapterGatewayInfo_.setAdapterIp(address_and_cidr[0]);
    }
    adapterGatewayInfo_.setDnsServers(QStringList() << wireGuardConfig->clientDnsAddress());
#endif

    setCurrentState(ConnectionState::CONNECTING);
    start(LowPriority);
 }

void WireGuardConnection::startDisconnect()
{
    if (isDisconnected()) {
        emit disconnected();
        return;
    }

    if (!kill_process_timer_.isActive())
        kill_process_timer_.start(PROCESS_KILL_TIMEOUT);

    adapterGatewayInfo_.clear();
    do_stop_thread_ = true;
}

bool WireGuardConnection::isDisconnected() const
{
    return getCurrentState() == ConnectionState::DISCONNECTED;
}

/*QString WireGuardConnection::getConnectedTapTunAdapterName()
{
    return pimpl_->getAdapterName();
}*/

// static
QString WireGuardConnection::getWireGuardExeName()
{
    return QString("windscribewireguard");
}

// static
QString WireGuardConnection::getWireGuardAdapterName()
{
#if defined(Q_OS_WIN)
    return QString("WindscribeWireGuard420");
#elif defined(Q_OS_MAC) || defined(Q_OS_LINUX)
    return QString("utun420");
#endif
}

void WireGuardConnection::run()
{
    WireGuardStatus status;
    quint64 bytesReceived = 0;
    quint64 bytesTransmitted = 0;
    bool is_configured = false;
    bool is_connected = false;

    BIND_CRASH_HANDLER_FOR_THREAD();

#if defined(Q_OS_WIN)
    qCDebug(LOG_WIREGUARD) << "Enable dns leak protection";
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
    Q_ASSERT(helper_win);
    helper_win->enableDnsLeaksProtection();
#endif

    for (pimpl_->connect();;) {
        if (do_stop_thread_) {
            pimpl_->disconnect();
            break;
        }
        const auto current_state = getCurrentState();
        unsigned int next_status_check_ms = 100u;
        if (current_state != ConnectionState::DISCONNECTED) {
            if (!pimpl_->getStatus(&status)) {
                qCDebug(LOG_WIREGUARD) << "Failed to get WireGuard status";
                pimpl_->disconnect();
                break;
            }
            switch (status.state) {
            case WireGuardState::NONE:
                // Daemon is not initialized.
                break;
            case WireGuardState::FAILURE:
                // Error state.
                qCDebug(LOG_WIREGUARD) << "WireGuard daemon error:" << status.errorCode;
#if defined(Q_OS_WIN)
                if(status.errorCode == 666u) {
                    // Stop daemon for sure before wintun driver reinstallation.
                    if(pimpl_->stopWireGuard()) {
                        setError(ProtoTypes::ConnectError::WINTUN_FATAL_ERROR);
                    }
                }
#endif
                do_stop_thread_ = true;
                break;
            case WireGuardState::STARTING:
                // Daemon is warming up, we have no other option that wait.
                break;
            case WireGuardState::LISTENING:
                // Daemon is accepting configuration.
                if (!is_configured) {
                    qCDebug(LOG_WIREGUARD) << "Configuring WireGuard...";
                    is_configured = true;
                    pimpl_->configure();
                    emit interfaceUpdated(pimpl_->getAdapterName());
                }
                break;
            case WireGuardState::CONNECTING:
                // Daemon is connecting (waiting for a handshake).
                next_status_check_ms = 250u;
                break;
            case WireGuardState::ACTIVE:
            {
                if (!is_connected) {
                    qCDebug(LOG_WIREGUARD) << "WireGuard daemon reported successful handshake";
                    is_connected = true;

#ifdef Q_OS_WIN
                    adapterGatewayInfo_ = AdapterUtils_win::getWindscribeConnectedAdapterInfo();
#endif
                    setCurrentStateAndEmitSignal(WireGuardConnection::ConnectionState::CONNECTED);
                }
                const auto newBytesReceived = status.bytesReceived - bytesReceived;
                const auto newBytesTransmitted = status.bytesTransmitted - bytesTransmitted;
                if (newBytesReceived || newBytesTransmitted) {
                    bytesReceived = status.bytesReceived;
                    bytesTransmitted = status.bytesTransmitted;
                    emit statisticsUpdated(newBytesReceived, newBytesTransmitted, false);
                }
                next_status_check_ms = 500u;
                break;
            }
            }
        }
        QThread::msleep(next_status_check_ms);
    }


#if defined(Q_OS_WIN)
    qCDebug(LOG_WIREGUARD) << "Disable dns leak protection";
    helper_win->disableDnsLeaksProtection();
#endif
}

void WireGuardConnection::onProcessKillTimeout()
{
    qCDebug(LOG_CONNECTION) << "WireGuard process not finished after "
                            << PROCESS_KILL_TIMEOUT << "ms";
    qCDebug(LOG_CONNECTION) << "kill the WireGuard process";
    kill_process_timer_.stop();
#if defined(Q_OS_WIN)
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
    helper_win->executeTaskKill(getWireGuardExeName());
#elif defined(Q_OS_MAC) || defined(Q_OS_LINUX)
    Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper_);
    helper_posix->executeRootCommand("pkill -f \"" + getWireGuardExeName() + "\"");
#elif defined(Q_OS_LINUX)
    Q_ASSERT(false);
#endif
}

WireGuardConnection::ConnectionState WireGuardConnection::getCurrentState() const
{
    QMutexLocker locker(&current_state_mutex_);
    return current_state_;
}

void WireGuardConnection::setCurrentState(ConnectionState state)
{
    QMutexLocker locker(&current_state_mutex_);
    current_state_ = state;
}

void WireGuardConnection::setCurrentStateAndEmitSignal(ConnectionState state)
{
    QMutexLocker locker(&current_state_mutex_);
    current_state_ = state;
    switch (current_state_) {
    case ConnectionState::DISCONNECTED:
        QTimer::singleShot(0, &kill_process_timer_, SLOT(stop()));
        emit disconnected();
        break;
    case ConnectionState::CONNECTED:        
        emit connected(adapterGatewayInfo_);
        break;
    default:
        break;
    }
}

void WireGuardConnection::setError(ProtoTypes::ConnectError err)
{
    QMutexLocker locker(&current_state_mutex_);
    current_state_ = ConnectionState::DISCONNECTED;
    do_stop_thread_ = true;
    emit error(err);
}
