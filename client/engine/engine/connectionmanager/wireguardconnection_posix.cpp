#include "wireguardconnection_posix.h"
#include "utils/ws_assert.h"
#include "utils/crashhandler.h"
#include "utils/logger.h"
#include "engine/helper/ihelper.h"
#include "types/enums.h"
#include "engine/wireguardconfig/wireguardconfig.h"
#include "types/wireguardtypes.h"
#include "engine/helper/helper_posix.h"

#ifdef Q_OS_LINUX
#include "engine/helper/helper_linux.h"
#endif

class WireGuardConnectionImpl
{
public:
    explicit WireGuardConnectionImpl(WireGuardConnection *host);
    void setConfig(const WireGuardConfig *wireGuardConfig);
    void setUsingKernelModule(bool usingKernelModule);
    void connect();
    void configure();
    void disconnect();
    bool getStatus(types::WireGuardStatus *status);
    bool stopWireGuard();

    QString getAdapterName() const { return adapterName_; }
 
private:
    WireGuardConnection *host_;
    QString adapterName_;
    WireGuardConfig config_;
    bool isStarted_;
    bool usingKernelModule_;
};

WireGuardConnectionImpl::WireGuardConnectionImpl(WireGuardConnection *host)
    : host_(host),
      adapterName_(WireGuardConnection::getWireGuardAdapterName()),
      isStarted_(false),
      usingKernelModule_(false)
{
}

void WireGuardConnectionImpl::setConfig(const WireGuardConfig *wireGuardConfig)
{
    if (wireGuardConfig)
        config_ = *wireGuardConfig;
}

void WireGuardConnectionImpl::connect()
{
    if (!isStarted_) {
        int retry = 0;
        IHelper::ExecuteError err;
        QString exeName;

        if (!usingKernelModule_)
            exeName = WireGuardConnection::getWireGuardExeName();

        while ((err = host_->helper_->startWireGuard(exeName, adapterName_)) != IHelper::EXECUTE_SUCCESS)
        {
            // don't bother another attempt if signature is invalid
            if (err == IHelper::EXECUTE_VERIFY_ERROR)
            {
                host_->setError(EXE_VERIFY_WIREGUARD_ERROR);
                return;
            }
            if (retry >= 2)
            {
                qCDebug(LOG_WIREGUARD) << "Can't start WireGuard after" << retry << "retries";
                host_->setError(WIREGUARD_CONNECTION_ERROR);
                return;
            }
            ++retry;
            QThread::msleep(1000);
        }
        qCDebug(LOG_WIREGUARD) << "WireGuard started after" << retry << "retries";
        isStarted_ = true;
    }
}

void WireGuardConnectionImpl::configure()
{
    WS_ASSERT(isStarted_);

    // Configure the client and the peer.
    if (!host_->helper_->configureWireGuard(config_)) {
        qCDebug(LOG_WIREGUARD) << "Failed to configure WireGuard";
        host_->setError(WIREGUARD_CONNECTION_ERROR);
    }
}

void WireGuardConnectionImpl::disconnect()
{
    if(!stopWireGuard())
        return;
    host_->setCurrentStateAndEmitSignal(WireGuardConnection::ConnectionState::DISCONNECTED);
}

bool WireGuardConnectionImpl::getStatus(types::WireGuardStatus *status)
{
    return isStarted_ && host_->helper_->getWireGuardStatus(status);
}

void WireGuardConnectionImpl::setUsingKernelModule(bool usingKernelModule)
{
    usingKernelModule_ = usingKernelModule;
}

bool WireGuardConnectionImpl::stopWireGuard()
{
    if (isStarted_) {
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
        isStarted_ = false;
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
                                       const QString &password, const types::ProxySettings &proxySettings,
                                       const WireGuardConfig *wireGuardConfig,
                                       bool isEnableIkev2Compression, bool isAutomaticConnectionMode,
                                       bool isCustomConfig)
{
    Q_UNUSED(configPathOrUrl);
    Q_UNUSED(ip);
    Q_UNUSED(dnsHostName);
    Q_UNUSED(username);
    Q_UNUSED(password);
    Q_UNUSED(proxySettings);
    Q_UNUSED(isEnableIkev2Compression);
    Q_UNUSED(isCustomConfig);

    qCDebug(LOG_CONNECTION) << "Connecting WireGuard:" << pimpl_->getAdapterName();

    do_stop_thread_ = true;
    wait();
    do_stop_thread_ = false;

    isAutomaticConnectionMode_ = isAutomaticConnectionMode;
    pimpl_->setConfig(wireGuardConfig);

    // if kernel module became available, or is no longer available, update the state
    using_kernel_module_ = checkForKernelModule();
    pimpl_->setUsingKernelModule(using_kernel_module_);

    // note: route gateway not used for WireGuard in AdapterGatewayInfo
    adapterGatewayInfo_.clear();
    adapterGatewayInfo_.setAdapterName(pimpl_->getAdapterName());
    QStringList address_and_cidr = wireGuardConfig->clientIpAddress().split('/');
    if (address_and_cidr.size() > 1)
    {
        adapterGatewayInfo_.setAdapterIp(address_and_cidr[0]);
    }
    adapterGatewayInfo_.setDnsServers(QStringList() << wireGuardConfig->clientDnsAddress());

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
    return QString("utun420");
}

void WireGuardConnection::run()
{
    types::WireGuardStatus status;
    quint64 bytesReceived = 0;
    quint64 bytesTransmitted = 0;
    bool is_configured = false;
    bool is_connected = false;
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    BIND_CRASH_HANDLER_FOR_THREAD();

    for (pimpl_->connect();;) {
        if (do_stop_thread_) {
            pimpl_->disconnect();
            break;
        }
        const auto current_state = getCurrentState();
        unsigned int next_status_check_ms = 100u;
        if (current_state != ConnectionState::DISCONNECTED) {

            if (current_state == ConnectionState::CONNECTED)
                elapsedTimer.invalidate();

            if (!pimpl_->getStatus(&status)) {
                qCDebug(LOG_WIREGUARD) << "Failed to get WireGuard status";
                pimpl_->disconnect();
                break;
            }
            switch (status.state) {
            case types::WireGuardState::NONE:
                // Not initialized.
                break;
            case types::WireGuardState::FAILURE:
                // Error state.
                qCDebug(LOG_WIREGUARD) << "WireGuard daemon error";
                do_stop_thread_ = true;
                break;
            case types::WireGuardState::STARTING:
                // Daemon is warming up, we have no other option that wait.
                break;
            case types::WireGuardState::LISTENING:
                // Accepting configuration.
                if (!is_configured) {
                    qCDebug(LOG_WIREGUARD) << "Configuring WireGuard...";
                    is_configured = true;
                    pimpl_->configure();
                    emit interfaceUpdated(pimpl_->getAdapterName());
                }
                break;
            case types::WireGuardState::CONNECTING:
                // Connecting (waiting for a handshake).
                next_status_check_ms = 250u;
                break;
            case types::WireGuardState::ACTIVE:
            {
                if (!is_connected) {
                    qCDebug(LOG_WIREGUARD) << "WireGuard daemon reported successful handshake";
                    is_connected = true;
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

        if (isAutomaticConnectionMode_ && elapsedTimer.isValid() && elapsedTimer.elapsed() >= kTimeoutForAutomatic) {
            setError(STATE_TIMEOUT_FOR_AUTOMATIC);
        }

        QThread::msleep(next_status_check_ms);
    }
}

void WireGuardConnection::onProcessKillTimeout()
{
    if (!using_kernel_module_)
    {
        qCDebug(LOG_CONNECTION) << "WireGuard process not finished after "
                                << PROCESS_KILL_TIMEOUT << "ms";
        qCDebug(LOG_CONNECTION) << "kill the WireGuard process";
        kill_process_timer_.stop();
        Helper_posix *helper_posix = dynamic_cast<Helper_posix *>(helper_);
        helper_posix->executeTaskKill(kTargetWireGuard);
    }
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

void WireGuardConnection::setError(CONNECT_ERROR err)
{
    QMutexLocker locker(&current_state_mutex_);
    current_state_ = ConnectionState::DISCONNECTED;
    do_stop_thread_ = true;
    emit error(err);
}

bool WireGuardConnection::checkForKernelModule()
{
#if defined(Q_OS_LINUX)
    Helper_linux *helper_linux = dynamic_cast<Helper_linux *>(helper_);
    return helper_linux->checkForWireGuardKernelModule();
#endif
    return false;
}
