#include <QScopeGuard>
#include <QStandardPaths>
#include <QTimer>

#include "wireguardconnection_win.h"
#include "wireguardringlogger.h"

#include "adapterutils_win.h"
#include "engine/types/wireguardconfig.h"
#include "engine/types/wireguardtypes.h"
#include "utils/crashhandler.h"
#include "utils/logger.h"

// Useful code:
// - mozilla-vpn-client\src\platforms\windows\daemon\wireguardutilswindows.cpp line 106 has code
//   for getting the interface LUID from the service name, rather than us having to hunt through
//   the registry.

// Design Notes:
// - IConnection::interfaceUpdated signal is not currently used in Engine::onConnectionManagerInterfaceUpdated
//   on Windows, so no need to emit it.

static const QString serviceIdentifier("WindscribeWireguard");

WireGuardConnection::WireGuardConnection(QObject *parent, IHelper *helper)
    : IConnection(parent),
      helper_(dynamic_cast<Helper_win*>(helper))
{
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

    QString configFile;
    connectedSignalEmited_ = false;

    auto guard = qScopeGuard([&]
    {
        serviceCtrlManager_.closeSCM();
        if (helper_ != nullptr && !helper_->stopWireGuard()) {
            qCDebug(LOG_CONNECTION) << "WireGuardConnection::startConnect - windscribe service failed to stop the wireguard service instance";
        }

        wireguardLog_.reset();

        // Ensure the config file is deleted if something goes awry during startup.  If all goes well,
        // the wireguard service will delete the file when it exits.
        if (!configFile.isEmpty() && QFile::exists(configFile)) {
            QFile::remove(configFile);
        }
    });

    try
    {
        if (helper_ == nullptr) {
            throw std::system_error(ERROR_INVALID_DATA, std::generic_category(),
                std::string("WireGuardConnection::startConnect - the helper pointer is null"));
        }

        if (wireGuardConfig == nullptr) {
            throw std::system_error(ERROR_INVALID_PARAMETER, std::generic_category(),
                std::string("WireGuardConnection::startConnect - the wireguard config parameter is null"));
        }

        qCDebug(LOG_CONNECTION) << "Starting" << getWireGuardExeName();

        quit();
        wait();

        // Design Note:
        // The wireguard embedded DLL service requires that the name of the configuration file we
        // create matches the name of the service the helper installs.  The helper will install
        // the service using the name WireGuardTunnel$ConfFileName

        configFile = tr("%1/%2.conf").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation), serviceIdentifier);

        if (!wireGuardConfig->generateConfigFile(configFile)) {
            throw std::system_error(0, std::generic_category(),
                std::string("WireGuardConfig::generateConfigFile could not create file ") + configFile.toStdString());
        }

        // Installing the wireguard service requires admin privilege.
        IHelper::ExecuteError err = helper_->startWireGuard(getWireGuardExeName(), configFile);
        if (err != IHelper::EXECUTE_SUCCESS)
        {
            if (err == IHelper::EXECUTE_VERIFY_ERROR)
            {
                emit error(ProtoTypes::ConnectError::EXE_VERIFY_WIREGUARD_ERROR);
                return;
            }

            throw std::system_error(0, std::generic_category(),
                std::string("Windscribe service could not install the wireguard service"));
        }

        // The wireguard service creates the log file in the same folder as the config file we passed to it.
        // We must create this log file watcher before we start the wireguard service to ensure we get
        // all log entries.
        QString logFile = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + tr("/log.bin");
        wireguardLog_.reset(new wsl::WireguardRingLogger(logFile));

        QString serviceName = tr("WireGuardTunnel$%1").arg(serviceIdentifier);

        serviceCtrlManager_.openSCM(SC_MANAGER_CONNECT);
        serviceCtrlManager_.openService(qPrintable(serviceName), SERVICE_QUERY_STATUS | SERVICE_START);
        serviceCtrlManager_.startService();

        // If the wireguard service indicates that it has started, the adapter and tunnel are up.
        // The only thing that may not have happened yet is the handshake between the wireguard service
        // and the server endpoint.  We'll check for the handshake in run().

        qCDebug(LOG_CONNECTION) << "WireGuard service started";

        guard.dismiss();

        start(LowPriority);
    }
    catch (std::system_error& ex)
    {
        qCDebug(LOG_CONNECTION) << ex.what();
        onWireguardServiceStartupFailure();
        emit error(ProtoTypes::ConnectError::WIREGUARD_CONNECTION_ERROR);
        emit disconnected();
    }
}

void WireGuardConnection::startDisconnect()
{
    quit();
}

bool WireGuardConnection::isDisconnected() const
{
    DWORD dwStatus = SERVICE_STOPPED;
    try
    {
        if (serviceCtrlManager_.isServiceOpen()) {
            dwStatus = serviceCtrlManager_.queryServiceStatus();
        }
    }
    catch (std::system_error& ex)
    {
        qCDebug(LOG_CONNECTION) << "WireGuardConnection::isDisconnected -" << ex.what();
    }

    return (dwStatus != SERVICE_RUNNING);
}

void WireGuardConnection::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();

    qCDebug(LOG_CONNECTION) << "WireGuardConnection::run - enable dns leak protection";
    helper_->enableDnsLeaksProtection();

    // See if the client-server handshake has happened yet.
    onGetWireguardLogUpdates();
    if (!connectedSignalEmited_) {
        onGetWireguardStats();
    }

    QScopedPointer< QTimer > timerGetWireguardStats(new QTimer);
    connect(timerGetWireguardStats.data(), &QTimer::timeout, this, &WireGuardConnection::onGetWireguardStats);
    timerGetWireguardStats->start(5000);

    QScopedPointer< QTimer > timerCheckServiceRunning(new QTimer);
    connect(timerCheckServiceRunning.data(), &QTimer::timeout, this, &WireGuardConnection::onCheckServiceRunning);
    timerCheckServiceRunning->start(2000);

    QScopedPointer< QTimer > timerGetWireguardLogUpdates(new QTimer);
    connect(timerGetWireguardLogUpdates.data(), &QTimer::timeout, this, &WireGuardConnection::onGetWireguardLogUpdates);
    timerGetWireguardLogUpdates->start(250);

    exec();

    timerGetWireguardStats->stop();
    timerCheckServiceRunning->stop();
    timerGetWireguardLogUpdates->stop();

    serviceCtrlManager_.closeSCM();

    // Get final receive/transmit byte counts.
    onGetWireguardStats();

    if (!helper_->stopWireGuard()) {
        qCDebug(LOG_CONNECTION) << "WireGuardConnection::run - windscribe service failed to stop the wireguard service instance";
    }

    wireguardLog_.reset();

    emit disconnected();

    qCDebug(LOG_CONNECTION) << "WireGuardConnection::run - disable dns leak protection";
    helper_->disableDnsLeaksProtection();
}

void WireGuardConnection::onCheckServiceRunning()
{
    if (isDisconnected())
    {
        qCDebug(LOG_CONNECTION) << "The wireguard service has stopped unexpectedly";
        quit();
    }
}

void WireGuardConnection::onGetWireguardLogUpdates()
{
    if (wireguardLog_.isNull()) {
        return;
    }

    wireguardLog_->getNewLogEntries();

    if (!connectedSignalEmited_ && wireguardLog_->isTunnelRunning())
    {
        connectedSignalEmited_ = true;
        AdapterGatewayInfo info = AdapterUtils_win::getWireguardConnectedAdapterInfo(serviceIdentifier);
        emit connected(info);
    }
}

void WireGuardConnection::onGetWireguardStats()
{
    // TODO: Need to periodically do this in order to retrieve the bytes received/transmitted
    // Would be nice if we could do this on-demand, as this information is only relevant when
    // the user wants to see it (see ConnectWindowItem::onConnectStateTextHoverEnter()).

    // We have to ask the helper to do this for us, as this process lacks permission to
    // access the API provided by the wireguard-nt kernel driver instance created by the
    // wireguard service.

    WireGuardStatus status;
    if (helper_->getWireGuardStatus(&status))
    {
        if (status.state == WireGuardState::ACTIVE)
        {
            if (!connectedSignalEmited_ && status.lastHandshake > 0)
            {
                connectedSignalEmited_ = true;
                AdapterGatewayInfo info = AdapterUtils_win::getWireguardConnectedAdapterInfo(serviceIdentifier);
                emit connected(info);
            }

            emit statisticsUpdated(status.bytesReceived, status.bytesTransmitted, true);
        }
    }
}

void WireGuardConnection::onWireguardServiceStartupFailure() const
{
    // Check if our wireguard service logged any startup errors.
    QFile file(qApp->applicationDirPath() + "/WireguardServiceLog.txt");
    if (file.exists())
    {
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            while (!file.atEnd())
            {
                QByteArray line = file.readLine();
                if (!line.isEmpty()) {
                    qCDebug(LOG_WIREGUARD) << line.trimmed();
                }
            }
        }
        else
        {
            qCDebug(LOG_CONNECTION) << "Could not open" << file.fileName();
        }
    }

    // Check if the wireguard-windows code logged anything.
    if (!wireguardLog_.isNull()) {
        wireguardLog_->getNewLogEntries();
    }
}

QString WireGuardConnection::getWireGuardExeName()
{
    return QString("WireguardService");
}

QString WireGuardConnection::getWireGuardAdapterName()
{
    return QString("WireGuardTunnel");
}
