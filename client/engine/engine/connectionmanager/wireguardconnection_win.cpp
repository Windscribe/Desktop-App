#include <QScopeGuard>
#include <QStandardPaths>
#include <QTimer>

#include "wireguardconnection_win.h"

#include "adapterutils_win.h"
#include "engine/wireguardconfig/wireguardconfig.h"
#include "types/wireguardtypes.h"
#include "utils/crashhandler.h"
#include "utils/logger.h"
#include "utils/servicecontrolmanager.h"
#include "utils/winutils.h"
#include "utils/ws_assert.h"

// Useful code:
// - mozilla-vpn-client\src\platforms\windows\daemon\wireguardutilswindows.cpp line 106 has code
//   for getting the interface LUID from the service name, rather than us having to hunt through
//   the registry.

// Design Notes:
// - Using Win32 kernel timers rather than QTimer in run().  This is due to the nature of QThread
//   inheritance: "It is important to remember that a QThread instance lives in the old thread that
//   instantiated it, not in the new thread that calls run(). This means that all of QThread's
//   queued slots and invoked methods will execute in the old thread.".  Thus, if we create a
//   QTimer in run() and connect to its timeout signal, the slot will be called by the thread that
//   created the QThread instance, not by the thread running run().
// - IConnection::interfaceUpdated signal is not currently used in Engine::onConnectionManagerInterfaceUpdated
//   on Windows, so no need to emit it.

static const QString kServiceIdentifier("WindscribeWireguard");
static const std::wstring kServiceName = L"WireGuardTunnel$" + kServiceIdentifier.toStdWString();

static HANDLE createTimer(int timeout, bool singleShot, PTIMERAPCROUTINE completionRoutine, LPVOID argToCompletionRoutine)
{
    HANDLE hTimer = ::CreateWaitableTimer(NULL, FALSE, NULL);
    if (hTimer == NULL) {
        qCDebug(LOG_CONNECTION) << "WireGuardConnection - CreateWaitableTimer failed:" << ::GetLastError();
        return NULL;
    }

    LARGE_INTEGER initialTimeout;
    initialTimeout.QuadPart = -(timeout * 10000);

    LONG period = (singleShot ? 0 : timeout);

    BOOL result = ::SetWaitableTimer(hTimer, &initialTimeout, period, completionRoutine, argToCompletionRoutine, FALSE);
    if (result == FALSE) {
        ::CloseHandle(hTimer);
        qCDebug(LOG_CONNECTION) << "WireGuardConnection - SetWaitableTimer failed:" << ::GetLastError();
        return NULL;
    }

    return hTimer;
}

static void cancelTimer(wsl::Win32Handle &timer)
{
    if (timer.isValid()) {
        ::CancelWaitableTimer(timer.getHandle());
        timer.closeHandle();
    }
}


WireGuardConnection::WireGuardConnection(QObject *parent, IHelper *helper)
    : IConnection(parent),
      helper_(dynamic_cast<Helper_win*>(helper))
{
    stopThreadEvent_.setHandle(::CreateEvent(NULL, TRUE, FALSE, NULL));
}

WireGuardConnection::~WireGuardConnection()
{
    if (isRunning()) {
        stop();
        wait();
    }
}

void WireGuardConnection::startConnect(const QString &configPathOrUrl, const QString &ip,
                                       const QString &dnsHostName, const QString &username,
                                       const QString &password, const types::ProxySettings &proxySettings,
                                       const WireGuardConfig *wireGuardConfig,
                                       bool isEnableIkev2Compression, bool isAutomaticConnectionMode,
                                       bool isCustomConfig, const QString &overrideDnsIp)
{
    Q_UNUSED(configPathOrUrl);
    Q_UNUSED(ip);
    Q_UNUSED(dnsHostName);
    Q_UNUSED(username);
    Q_UNUSED(password);
    Q_UNUSED(proxySettings);
    Q_UNUSED(isEnableIkev2Compression);
    Q_UNUSED(isCustomConfig);
    Q_UNUSED(overrideDnsIp);

    WS_ASSERT(helper_ != nullptr);
    WS_ASSERT(wireGuardConfig != nullptr);
    WS_ASSERT(stopThreadEvent_.isValid());

    if (isRunning()) {
        stop();
        wait();
    }

    connectedSignalEmited_ = false;
    isAutomaticConnectionMode_ = isAutomaticConnectionMode;
    wireGuardConfig_ = wireGuardConfig;
    ::ResetEvent(stopThreadEvent_.getHandle());

    start(LowPriority);
}

void WireGuardConnection::startDisconnect()
{
    if (isRunning()) {
        stop();
    }
    else if (isDisconnected()) {
        emit disconnected();
    }
}

bool WireGuardConnection::isDisconnected() const
{
    DWORD dwStatus = SERVICE_STOPPED;
    try {
        wsl::ServiceControlManager scm;
        scm.openSCM(SC_MANAGER_CONNECT);
        if (scm.isServiceInstalled(kServiceName.c_str())) {
            scm.openService(kServiceName.c_str(), SERVICE_QUERY_STATUS);
            dwStatus = scm.queryServiceStatus();
        }
    }
    catch (std::system_error& ex) {
        qCDebug(LOG_CONNECTION) << "WireGuardConnection::isDisconnected -" << ex.what();
    }

    return (dwStatus == SERVICE_STOPPED || dwStatus == SERVICE_STOP_PENDING);
}

void WireGuardConnection::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();

    qCDebug(LOG_CONNECTION) << "Starting" << getWireGuardExeName();

    // Design Notes:
    // The wireguard embedded DLL service requires that the name of the configuration file we
    // create matches the name of the service the helper installs.  The helper will install
    // the service using the name WireGuardTunnel$ConfFileName

    QString configFile = QString("%1/%2.conf").arg(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation), kServiceIdentifier);

    // Installing the wireguard service requires admin privilege.
    IHelper::ExecuteError err = helper_->startWireGuard(getWireGuardExeName(), configFile);
    if (err != IHelper::EXECUTE_SUCCESS)
    {
        qCDebug(LOG_CONNECTION) << "Windscribe service could not install the WireGuard service";
        emit error((err == IHelper::EXECUTE_VERIFY_ERROR ? CONNECT_ERROR::EXE_VERIFY_WIREGUARD_ERROR
                                                         : CONNECT_ERROR::WIREGUARD_CONNECTION_ERROR));
        emit disconnected();
        return;
    }

    auto stopWireGuard = qScopeGuard([&] {
        helper_->stopWireGuard();
    });

    // If there was a running instance of the wireguard service, the helper (startWireGuard call) will
    // have stopped it and it will have deleted the existing config file.  Therefore, don't create our
    // new config file until we're sure the wireguard service is stopped.
    if (!wireGuardConfig_->generateConfigFile(configFile)) {
        emit error(CONNECT_ERROR::WIREGUARD_CONNECTION_ERROR);
        emit disconnected();
        return;
    }

    // The wireguard service creates the log file in the same folder as the config file we passed to it.
    // We must create this log file watcher before we start the wireguard service to ensure we get
    // all log entries.
    QString logFile = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QString("/log.bin");
    wireguardLog_.reset(new wsl::WireguardRingLogger(logFile));

    bool disableDNSLeakProtection = false;

    bool serviceStarted = startService();

    while (serviceStarted) {
        helper_->enableDnsLeaksProtection();
        disableDNSLeakProtection = true;

        // If the wireguard service indicates that it has started, the adapter and tunnel are up.
        // Let's check if the client-server handshake, which indicates the tunnel is good-to-go, has happened yet.
        // onGetWireguardLogUpdates() is much less 'expensive' than calling onGetWireguardStats().
        onGetWireguardLogUpdates();
        if (!connectedSignalEmited_) {
            onGetWireguardStats();
        }

        wsl::Win32Handle timerGetWireguardStats(createTimer(kTimeoutForGetStats, false, getWireguardStatsProc, this));
        if (!timerGetWireguardStats.isValid()) {
            break;
        }

        wsl::Win32Handle timerCheckServiceRunning(createTimer(kTimeoutForCheckService, false, checkServiceRunningProc, this));
        if (!timerCheckServiceRunning.isValid()) {
            break;
        }

        wsl::Win32Handle timerTimeoutForAutomatic;
        if (isAutomaticConnectionMode_) {
            timerTimeoutForAutomatic.setHandle(createTimer(kTimeoutForAutomatic, true, automaticConnectionTimeoutProc, this));
            if (!timerTimeoutForAutomatic.isValid()) {
                break;
            }
        }

        wsl::Win32Handle timerGetWireguardLogUpdates(createTimer(kTimeoutForLogUpdate, false, getWireguardLogUpdatesProc, this));
        if (!timerGetWireguardLogUpdates.isValid()) {
            break;
        }

        while (true) {
            DWORD result = ::WaitForSingleObjectEx(stopThreadEvent_.getHandle(), INFINITE, TRUE);
            if (result != WAIT_IO_COMPLETION) {
                break;
            }
        }

        cancelTimer(timerGetWireguardLogUpdates);
        cancelTimer(timerGetWireguardStats);
        cancelTimer(timerCheckServiceRunning);
        cancelTimer(timerTimeoutForAutomatic);

        // Get final receive/transmit byte counts.
        onGetWireguardStats();

        break;
    }

    if (helper_->stopWireGuard()) {
        configFile.clear();
    }
    else {
        qCDebug(LOG_CONNECTION) << "WireGuardConnection::run - windscribe service failed to stop the WireGuard service instance";
    }

    stopWireGuard.dismiss();

    wireguardLog_->getFinalLogEntries();

    // Ensure the config file is deleted if something went awry during service startup.  If all goes well,
    // the wireguard service will delete the file when it exits.
    if (!configFile.isEmpty() && QFile::exists(configFile)) {
        QFile::remove(configFile);
    }

    if (disableDNSLeakProtection) {
        helper_->disableDnsLeaksProtection();
    }

    // Delay emiting signals until we have cleaned up all our resources.
    if (wireguardLog_->adapterSetupFailed()) {
        emit error(CONNECT_ERROR::WIREGUARD_ADAPTER_SETUP_FAILED);
    }
    else if (!serviceStarted) {
        emit error(CONNECT_ERROR::WIREGUARD_CONNECTION_ERROR);
    }

    emit disconnected();

    qCDebug(LOG_CONNECTION) << "WireGuardConnection::run exiting";
}

void WireGuardConnection::stop()
{
    if (stopThreadEvent_.isValid()) {
        BOOL result = ::SetEvent(stopThreadEvent_.getHandle());
        if (result == FALSE) {
            qCDebug(LOG_CONNECTION) << "WireGuardConnection::stop - SetEvent failed:" << ::GetLastError();
        }
    }
}

void WireGuardConnection::onCheckServiceRunning()
{
    if (isDisconnected()) {
        qCDebug(LOG_CONNECTION) << "The WireGuard service has stopped unexpectedly";
        stop();
    }
}

void WireGuardConnection::onGetWireguardLogUpdates()
{
    if (!wireguardLog_.isNull()) {
        wireguardLog_->getNewLogEntries();

        if (!connectedSignalEmited_ && wireguardLog_->isTunnelRunning()) {
            onTunnelConnected();
        }

        // We must rely on the WireGuard service log to detect handshake failures.  The service itself does
        // not provide a mechanism for detecting such a failure.
        if (wireguardLog_->isTunnelRunning() && wireguardLog_->handshakeFailed()) {
            onWireguardHandshakeFailure();
        }
    }
}

void WireGuardConnection::onGetWireguardStats()
{
    // We have to ask the helper to do this for us, as this process lacks permission to
    // access the API provided by the wireguard-nt kernel driver instance created by the
    // wireguard service.

    types::WireGuardStatus status;
    if (helper_->getWireGuardStatus(&status)) {
        if (status.state == types::WireGuardState::ACTIVE)
        {
            if (!connectedSignalEmited_ && status.lastHandshake > 0) {
                onTunnelConnected();
            }

            emit statisticsUpdated(status.bytesReceived, status.bytesTransmitted, true);
        }
    }
}

void WireGuardConnection::onAutomaticConnectionTimeout()
{
    if (!connectedSignalEmited_) {
        emit error(CONNECT_ERROR::STATE_TIMEOUT_FOR_AUTOMATIC);
        stop();
    }
}

void WireGuardConnection::onWireguardHandshakeFailure()
{
    auto haveInternet = WinUtils::haveInternetConnectivity();
    if (!haveInternet.has_value()) {
        qCDebug(LOG_CONNECTION) << "The WireGuard service reported a handshake failure, but the Internet connectivity check failed.";
        return;
    }

    if (*haveInternet) {
        types::WireGuardStatus status;
        if (helper_->getWireGuardStatus(&status) && (status.state == types::WireGuardState::ACTIVE) && (status.lastHandshake > 0)) {
            // The handshake should occur every ~2 minutes.  After 3 minutes, the server will discard our key
            // information and will silently reject anything we send to it until we make another wgconfig API call.
            QDateTime lastHandshake = QDateTime::fromSecsSinceEpoch((status.lastHandshake / 10000000) - 11644473600LL, Qt::UTC);
            qint64 secsTo = lastHandshake.secsTo(QDateTime::currentDateTimeUtc());

            if (secsTo >= 3*60) {
                qCDebug(LOG_CONNECTION) << secsTo << "seconds have passed since the last WireGuard handshake, disconnecting the tunnel.";
                stop();
            }
        }
    }
    else {
        qCDebug(LOG_CONNECTION) << "The WireGuard service reported a handshake failure and Windows reports no Internet connectivity, disconnecting the tunnel.";
        stop();
    }
}

bool WireGuardConnection::startService()
{
    if (stopThreadEvent_.isSignaled()) {
        return false;
    }

    try {
        wsl::ServiceControlManager scm;
        scm.openSCM(SC_MANAGER_CONNECT);
        scm.openService(kServiceName.c_str(), SERVICE_QUERY_STATUS | SERVICE_START);
        scm.startService();
        return true;
    }
    catch (std::system_error& ex) {
        qCDebug(LOG_CONNECTION) << ex.what();
    }

    return false;
}

void WireGuardConnection::onTunnelConnected()
{
    connectedSignalEmited_ = true;
    AdapterGatewayInfo info = AdapterUtils_win::getWireguardConnectedAdapterInfo(kServiceIdentifier);
    emit connected(info);
}

void CALLBACK WireGuardConnection::checkServiceRunningProc(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue)
{
    Q_UNUSED(dwTimerLowValue)
    Q_UNUSED(dwTimerHighValue)
    WireGuardConnection* wc = (WireGuardConnection*)lpArgToCompletionRoutine;
    wc->onCheckServiceRunning();
}

void CALLBACK WireGuardConnection::getWireguardLogUpdatesProc(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue)
{
    Q_UNUSED(dwTimerLowValue)
    Q_UNUSED(dwTimerHighValue)
    WireGuardConnection* wc = (WireGuardConnection*)lpArgToCompletionRoutine;
    wc->onGetWireguardLogUpdates();
}

void CALLBACK WireGuardConnection::getWireguardStatsProc(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue)
{
    Q_UNUSED(dwTimerLowValue)
    Q_UNUSED(dwTimerHighValue)
    WireGuardConnection* wc = (WireGuardConnection*)lpArgToCompletionRoutine;
    wc->onGetWireguardStats();
}

void CALLBACK WireGuardConnection::automaticConnectionTimeoutProc(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue)
{
    Q_UNUSED(dwTimerLowValue)
    Q_UNUSED(dwTimerHighValue)
    WireGuardConnection* wc = (WireGuardConnection*)lpArgToCompletionRoutine;
    wc->onAutomaticConnectionTimeout();
}
