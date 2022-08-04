#include "engineserver.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "ipc/server.h"
#include "ipc/protobufcommand.h"
#include "ipc/servercommands.h"
#include "ipc/clientcommands.h"
#include "engine/openvpnversioncontroller.h"
#include <QDateTime>

#ifdef Q_OS_WIN
    #include "engine/taputils/tapinstall_win.h"
#endif

EngineServer::EngineServer(QObject *parent) : QObject(parent)
  , server_(NULL)
  , engine_(NULL)
  , threadEngine_(NULL)
  , bClientAuthReceived_(false)
{
    curEngineSettings_.loadFromSettings();
}

EngineServer::~EngineServer()
{
    curEngineSettings_.saveToSettings();

    const auto connectionKeys = connections_.keys();
    for (auto connection : connectionKeys)
    {
        dynamic_cast<QObject*>(connection)->disconnect();
        connections_.remove(connection);
        SAFE_DELETE(connection);
    }
    SAFE_DELETE(server_);
    qCDebug(LOG_IPC) << "IPC server stopped";
}

void EngineServer::run()
{
    Q_ASSERT(server_ == NULL);
    server_ = new IPC::Server();
    connect(dynamic_cast<QObject*>(server_), SIGNAL(newConnection(IPC::IConnection *)), SLOT(onServerCallbackAcceptFunction(IPC::IConnection *)), Qt::QueuedConnection);

    if (!server_->start())
    {
        qCDebug(LOG_IPC) << "Can't start IPC server, exit";
        Q_EMIT finished();
    }
    else
    {
        qCDebug(LOG_IPC) << "IPC server started";
    }
}

bool EngineServer::handleCommand(IPC::Command *command)
{
    if (command->getStringId() == IPC::ClientCommands::Init::getCommandStringId())
    {
        qCDebug(LOG_IPC) << "Received Init";

        // Q_ASSERT(engine_ == NULL);
        // Q_ASSERT(threadEngine_ == NULL);

        if (engine_ == NULL && threadEngine_ == NULL)
        {
            qCDebug(LOG_IPC) << "Building engine";
            threadEngine_ = new QThread(this);
            engine_ = new Engine(curEngineSettings_);
            engine_->moveToThread(threadEngine_);
            connect(threadEngine_, &QThread::started, engine_, &Engine::init);
            connect(engine_, &Engine::cleanupFinished, threadEngine_, &QThread::quit);
            connect(threadEngine_, &QThread::finished, this,  &EngineServer::onEngineCleanupFinished);

            connect(engine_, &Engine::initFinished, this, &EngineServer::onEngineInitFinished);
            connect(engine_, &Engine::bfeEnableFinished, this, &EngineServer::onEngineBfeEnableFinished);
            connect(engine_, &Engine::firewallStateChanged, this, &EngineServer::onEngineFirewallStateChanged);
            connect(engine_, &Engine::loginFinished, this, &EngineServer::onEngineLoginFinished);
            connect(engine_, &Engine::loginStepMessage, this, &EngineServer::onEngineLoginMessage);
            connect(engine_, &Engine::notificationsUpdated, this, &EngineServer::onEngineNotificationsUpdated);
            connect(engine_, &Engine::checkUpdateUpdated, this, &EngineServer::onEngineCheckUpdateUpdated);
            connect(engine_, &Engine::updateVersionChanged, this, &EngineServer::onEngineUpdateVersionChanged);
            connect(engine_, &Engine::myIpUpdated, this, &EngineServer::onEngineMyIpUpdated);
            connect(engine_, &Engine::sessionStatusUpdated, this, &EngineServer::onEngineUpdateSessionStatus);
            connect(engine_, &Engine::sessionDeleted, this, &EngineServer::onEngineSessionDeleted);
            connect(engine_->getConnectStateController(), &IConnectStateController::stateChanged, this, &EngineServer::onEngineConnectStateChanged);
            connect(engine_, &Engine::protocolPortChanged, this, &EngineServer::onEngineProtocolPortChanged);
            connect(engine_, &Engine::statisticsUpdated, this, &EngineServer::onEngineStatisticsUpdated);
            connect(engine_, &Engine::emergencyConnected, this, &EngineServer::onEngineEmergencyConnected);
            connect(engine_, &Engine::emergencyDisconnected, this, &EngineServer::onEngineEmergencyDisconnected);
            connect(engine_, &Engine::emergencyConnectError, this, &EngineServer::onEngineEmergencyConnectError);
            connect(engine_, &Engine::testTunnelResult, this, &EngineServer::onEngineTestTunnelResult);
            connect(engine_, &Engine::lostConnectionToHelper, this, &EngineServer::onEngineLostConnectionToHelper);
            connect(engine_, &Engine::proxySharingStateChanged, this, &EngineServer::onEngineProxySharingStateChanged);
            connect(engine_, &Engine::wifiSharingStateChanged, this, &EngineServer::onEngineWifiSharingStateChanged);
            connect(engine_, &Engine::vpnSharingConnectedWifiUsersCountChanged, this, &EngineServer::onEngineConnectedWifiUsersCountChanged);
            connect(engine_, &Engine::vpnSharingConnectedProxyUsersCountChanged, this, &EngineServer::onEngineConnectedProxyUsersCountChanged);
            connect(engine_, &Engine::signOutFinished, this, &EngineServer::onEngineSignOutFinished);
            connect(engine_, &Engine::gotoCustomOvpnConfigModeFinished, this, &EngineServer::onEngineGotoCustomOvpnConfigModeFinished);
            connect(engine_, &Engine::detectionCpuUsageAfterConnected, this, &EngineServer::onEngineDetectionCpuUsageAfterConnected);
            connect(engine_, &Engine::requestUsername, this, &EngineServer::onEngineRequestUsername);
            connect(engine_, &Engine::requestPassword, this, &EngineServer::onEngineRequestPassword);
            connect(engine_, &Engine::networkChanged, this, &EngineServer::onEngineNetworkChanged);
            connect(engine_, &Engine::confirmEmailFinished, this, &EngineServer::onEngineConfirmEmailFinished);
            connect(engine_, &Engine::sendDebugLogFinished, this, &EngineServer::onEngineSendDebugLogFinished);
            connect(engine_, &Engine::webSessionToken, this, &EngineServer::onEngineWebSessionToken);
            connect(engine_, &Engine::macAddrSpoofingChanged, this, &EngineServer::onMacAddrSpoofingChanged);
            connect(engine_, &Engine::sendUserWarning, this, &EngineServer::onEngineSendUserWarning);
            connect(engine_, &Engine::internetConnectivityChanged, this, &EngineServer::onEngineInternetConnectivityChanged);
            connect(engine_, &Engine::packetSizeChanged, this, &EngineServer::onEnginePacketSizeChanged);
            connect(engine_, &Engine::packetSizeDetectionStateChanged, this, &EngineServer::onEnginePacketSizeDetectionStateChanged);
            connect(engine_, &Engine::hostsFileBecameWritable, this, &EngineServer::onHostsFileBecameWritable);
            connect(engine_, &Engine::wireGuardAtKeyLimit, this, &EngineServer::wireGuardAtKeyLimit);
            connect(this, &EngineServer::wireGuardKeyLimitUserResponse, engine_, &Engine::onWireGuardKeyLimitUserResponse);
            connect(engine_, &Engine::loginError, this, &EngineServer::onEngineLoginError);
            threadEngine_->start(QThread::LowPriority);
        }
        else
        {
            qCDebug(LOG_IPC) << "Engine already built";
        }

        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::EnableBfe_win::getCommandStringId())
    {
        engine_->enableBFE_win();
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::Cleanup::getCommandStringId())
    {
        IPC::ClientCommands::Cleanup *cleanupCmd = static_cast<IPC::ClientCommands::Cleanup *>(command);

        if (engine_ == NULL)
        {
            Q_EMIT finished();
        }
        else
        {
            engine_->cleanup(cleanupCmd->isExitWithRestart_, cleanupCmd->isFirewallChecked_,
                             cleanupCmd->isFirewallAlwaysOn_, cleanupCmd->isLaunchOnStartup_);
        }
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::Firewall::getCommandStringId())
    {
        IPC::ClientCommands::Firewall *firewallCmd = static_cast<IPC::ClientCommands::Firewall *>(command);
        if (firewallCmd->isEnable_)
        {
            engine_->firewallOn();
        }
        else
        {
            engine_->firewallOff();
        }
        return true;
    }

    else if (command->getStringId() == IPC::ClientCommands::Login::getCommandStringId())
    {
        IPC::ClientCommands::Login *loginCmd = static_cast<IPC::ClientCommands::Login *>(command);

        // hide password for logging
        //IPC::ProtobufCommand<IPCClientCommands::Login> loggedCmd = *loginCmd;
        //loggedCmd.getProtoObj().set_password("*****");
        //qCDebugMultiline(LOG_IPC) << QString::fromStdString(loggedCmd.getDebugString());

        // login with last login settings
        if (loginCmd->useLastLoginSettings == true)
        {
            engine_->loginWithLastLoginSettings();
        }
        // login with auth hash
        else if (!loginCmd->authHash_.isEmpty())
        {
            engine_->loginWithAuthHash(loginCmd->authHash_);
        }
        // login with username and password
        else if (!loginCmd->username_.isEmpty() && !loginCmd->password_.isEmpty())
        {
            engine_->loginWithUsernameAndPassword(loginCmd->username_, loginCmd->password_, loginCmd->code2fa_);
        }
        else
        {
            Q_ASSERT(false);
            return false;
        }
    }
    else if (command->getStringId() == IPC::ClientCommands::ApplicationActivated::getCommandStringId())
    {
        IPC::ClientCommands::ApplicationActivated *appActivatedCmd = static_cast<IPC::ClientCommands::ApplicationActivated *>(command);
        if (appActivatedCmd->isActivated_)
        {
            engine_->applicationActivated();
        }
        else
        {
            engine_->applicationDeactivated();
        }
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::Connect::getCommandStringId())
    {
        IPC::ClientCommands::Connect *connectCmd = static_cast<IPC::ClientCommands::Connect *>(command);
        engine_->connectClick(connectCmd->locationId_);
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::Disconnect::getCommandStringId())
    {
        engine_->disconnectClick();
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::StartProxySharing::getCommandStringId())
    {
        IPC::ClientCommands::StartProxySharing *cmd = static_cast<IPC::ClientCommands::StartProxySharing *>(command);
        engine_->startProxySharing(cmd->sharingMode_);
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::StopProxySharing::getCommandStringId())
    {
        engine_->stopProxySharing();
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::StartWifiSharing::getCommandStringId())
    {
        IPC::ClientCommands::StartWifiSharing *cmd = static_cast<IPC::ClientCommands::StartWifiSharing *>(command);
        engine_->startWifiSharing(cmd->ssid_, cmd->password_);
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::StopWifiSharing::getCommandStringId())
    {
        engine_->stopWifiSharing();
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::EmergencyConnect::getCommandStringId())
    {
        engine_->emergencyConnectClick();
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::EmergencyDisconnect::getCommandStringId())
    {
        engine_->emergencyDisconnectClick();
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::SignOut::getCommandStringId())
    {
        IPC::ClientCommands::SignOut *cmd = static_cast<IPC::ClientCommands::SignOut *>(command);
        engine_->signOut(cmd->isKeepFirewallOn_);
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::SendDebugLog::getCommandStringId())
    {
        engine_->sendDebugLog();
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::GetWebSessionToken::getCommandStringId())
    {
        IPC::ClientCommands::GetWebSessionToken *cmd = static_cast<IPC::ClientCommands::GetWebSessionToken *>(command);
        engine_->getWebSessionToken(cmd->purpose_);
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::SendConfirmEmail::getCommandStringId())
    {
        engine_->sendConfirmEmail();
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::RecordInstall::getCommandStringId())
    {
        engine_->recordInstall();
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::GetIpv6StateInOS::getCommandStringId())
    {
        IPC::ServerCommands::Ipv6StateInOS cmd;
        cmd.isEnabled_ = engine_->IPv6StateInOS();
        sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::SetIpv6StateInOS::getCommandStringId())
    {
        IPC::ClientCommands::SetIpv6StateInOS *cmd = static_cast<IPC::ClientCommands::SetIpv6StateInOS *>(command);
        engine_->setIPv6EnabledInOS(cmd->isEnabled_);
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::SplitTunneling::getCommandStringId())
    {
        IPC::ClientCommands::SplitTunneling *cmd = static_cast<IPC::ClientCommands::SplitTunneling *>(command);

        bool isActive = cmd->splitTunneling_.settings.active;
        bool isExclude = cmd->splitTunneling_.settings.mode == SPLIT_TUNNELING_MODE_EXCLUDE;

        QStringList files;
        for (int i = 0; i < cmd->splitTunneling_.apps.size(); ++i)
        {
            if (cmd->splitTunneling_.apps[i].active)
            {
                files << cmd->splitTunneling_.apps[i].fullName;
            }
        }

        QStringList ips;
        QStringList hosts;
        for (int i = 0; i < cmd->splitTunneling_.networkRoutes.size(); ++i)
        {
            if (cmd->splitTunneling_.networkRoutes[i].type == SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP)
            {
                ips << cmd->splitTunneling_.networkRoutes[i].name;
            }
            else if (cmd->splitTunneling_.networkRoutes[i].type == SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_HOSTNAME)
            {
                hosts << cmd->splitTunneling_.networkRoutes[i].name;
            }
        }

        engine_->setSplitTunnelingSettings(isActive, isExclude, files, ips, hosts);
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::GotoCustomOvpnConfigMode::getCommandStringId())
    {
        engine_->gotoCustomOvpnConfigMode();
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::SetSettings::getCommandStringId())
    {
        IPC::ClientCommands::SetSettings *setSettingsCmd = static_cast<IPC::ClientCommands::SetSettings *>(command);

        if (curEngineSettings_ != setSettingsCmd->getEngineSettings())
        {
            curEngineSettings_ = setSettingsCmd->getEngineSettings();
            if (engine_)
            {
                engine_->setSettings(curEngineSettings_);
            }
            curEngineSettings_.saveToSettings();
        }
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::SetBlockConnect::getCommandStringId())
    {
        IPC::ClientCommands::SetBlockConnect *blockConnectCmd = static_cast<IPC::ClientCommands::SetBlockConnect *>(command);
        if (engine_->isBlockConnect() != blockConnectCmd->isBlockConnect_)
        {
            qCDebugMultiline(LOG_IPC) << QString::fromStdString(command->getDebugString());
            engine_->setBlockConnect(blockConnectCmd->isBlockConnect_);
        }
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::ClearCredentials::getCommandStringId())
    {
        engine_->clearCredentials();
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::SpeedRating::getCommandStringId())
    {
        IPC::ClientCommands::SpeedRating *cmd = static_cast<IPC::ClientCommands::SpeedRating *>(command);
        engine_->speedRating(cmd->rating_, cmd->localExternalIp_);
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::ContinueWithCredentialsForOvpnConfig::getCommandStringId())
    {
        IPC::ClientCommands::ContinueWithCredentialsForOvpnConfig *cmd = static_cast<IPC::ClientCommands::ContinueWithCredentialsForOvpnConfig *>(command);
        engine_->continueWithUsernameAndPassword(cmd->username_, cmd->password_, cmd->isSave_);
        return true;
    }
    else if (command->getStringId() == IPC::ClientCommands::DetectPacketSize::getCommandStringId())
    {
        engine_->detectAppropriatePacketSize();
    }
    else if (command->getStringId() == IPC::ClientCommands::UpdateVersion::getCommandStringId())
    {
        IPC::ClientCommands::UpdateVersion *cmd = static_cast<IPC::ClientCommands::UpdateVersion *>(command);
        if (cmd->cancelDownload_)
        {
            engine_->stopUpdateVersion();
        }
        else
        {
            engine_->updateVersion(cmd->hwnd_);
        }
    }
    else if (command->getStringId() == IPC::ClientCommands::UpdateWindowInfo::getCommandStringId())
    {
        IPC::ClientCommands::UpdateWindowInfo *cmd = static_cast<IPC::ClientCommands::UpdateWindowInfo *>(command);
        engine_->updateWindowInfo(cmd->windowCenterX_, cmd->windowCenterY_);
    }
    else if (command->getStringId() == IPC::ClientCommands::MakeHostsWritableWin::getCommandStringId()) {
#ifdef Q_OS_WIN
        engine_->makeHostsFileWritableWin();
#endif
    }
    else if (command->getStringId() == IPC::ClientCommands::AdvancedParametersChanged::getCommandStringId())
    {
        engine_->updateAdvancedParams();
    }


    return false;
}

void EngineServer::sendEngineInitReturnCode(ENGINE_INIT_RET_CODE retCode)
{
    if (retCode == ENGINE_INIT_SUCCESS)
    {
        connect(engine_->getLocationsModel(), SIGNAL(locationsUpdated(LocationID, QString, QSharedPointer<QVector<types::LocationItem> >)),
                SLOT(onEngineLocationsModelItemsUpdated(LocationID, QString, QSharedPointer<QVector<types::LocationItem> >)));
        connect(engine_->getLocationsModel(), SIGNAL(bestLocationUpdated(LocationID)),
                SLOT(onEngineLocationsModelBestLocationUpdated(LocationID)));
        connect(engine_->getLocationsModel(), SIGNAL(customConfigsLocationsUpdated(QSharedPointer<QVector<types::LocationItem> >)),
                SLOT(onEngineLocationsModelCustomConfigItemsUpdated(QSharedPointer<QVector<types::LocationItem> >)));
        connect(engine_->getLocationsModel(), SIGNAL(locationPingTimeChanged(LocationID,PingTime)),
                SLOT(onEngineLocationsModelPingChangedChanged(LocationID,PingTime)));

        IPC::ServerCommands::InitFinished cmd(INIT_STATE_SUCCESS, curEngineSettings_, OpenVpnVersionController::instance().getAvailableOpenVpnVersions(),
                                              engine_->isWifiSharingSupported(), engine_->isApiSavedSettingsExists(), engine_->getAuthHash());

        sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);

        engine_->updateCurrentInternetConnectivity();
    }
    else if (retCode == ENGINE_INIT_HELPER_FAILED)
    {
        IPC::ServerCommands::InitFinished cmd(INIT_STATE_HELPER_FAILED);
        sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
    }
    else if (retCode == ENGINE_INIT_BFE_SERVICE_FAILED)
    {
        IPC::ServerCommands::InitFinished cmd(INIT_STATE_BFE_SERVICE_NOT_STARTED);
        sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
    }
    else if (retCode == ENGINE_INIT_HELPER_USER_CANCELED)
    {
        IPC::ServerCommands::InitFinished cmd(INIT_STATE_HELPER_USER_CANCELED);

        sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
    }
    else
    {
        Q_ASSERT(false);
    }
}

void EngineServer::onServerCallbackAcceptFunction(IPC::IConnection *connection)
{
    qCDebug(LOG_IPC) << "Client connected:" << connection;

    connections_.insert(connection, ClientConnectionDescr());

    connect(dynamic_cast<QObject*>(connection), SIGNAL(newCommand(IPC::Command *, IPC::IConnection *)), SLOT(onConnectionCommandCallback(IPC::Command *, IPC::IConnection *)), Qt::QueuedConnection);
    connect(dynamic_cast<QObject*>(connection), SIGNAL(stateChanged(int, IPC::IConnection *)), SLOT(onConnectionStateCallback(int, IPC::IConnection *)), Qt::QueuedConnection);
}

void EngineServer::sendCommand(IPC::Command *command)
{
    if ((command->getStringId() != IPC::ClientCommands::Login::getCommandStringId()) &&
        (command->getStringId() != IPC::ClientCommands::SetBlockConnect::getCommandStringId()))
    {
        // The SetBlockConnect command is received every minute.  handleCommand will log it if the value
        // has changed, so that we don't flood the log with this entry when the app is up for an
        // extended period of time.
        qCDebugMultiline(LOG_IPC) << QString::fromStdString(command->getDebugString());
    }


    // check if the client is made authorization
    if (bClientAuthReceived_)
    {
        handleCommand(command);
    }
    else
    {
        // wait for command ClientAuth for authorization of client
        if (command->getStringId() == IPC::ClientCommands::ClientAuth::getCommandStringId())
        {
            bClientAuthReceived_ = true;
            IPC::ServerCommands::AuthReply cmdReply;
            emitCommand(&cmdReply);
        }
    }
}

void EngineServer::onConnectionStateCallback(int state, IPC::IConnection *connection)
{
    if (state == IPC::CONNECTION_DISCONNECTED || state == IPC::CONNECTION_ERROR)
    {
        qCDebug(LOG_IPC) << "Client disconnected:" << connection;

        int removed = connections_.remove(connection);
        if (removed > 0)
        {
            dynamic_cast<QObject*>(connection)->disconnect();
            SAFE_DELETE(connection);
        }

        // close engine, if all of the clients are disconnected
        if (connections_.isEmpty())
        {
            qCDebug(LOG_IPC) << "All of the clients are disconnected";
            Q_EMIT finished();
        }
    }
    else
    {
        Q_ASSERT(false);
    }
}

void EngineServer::onEngineCleanupFinished()
{
    IPC::ServerCommands::CleanupFinished cmd;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineInitFinished(ENGINE_INIT_RET_CODE retCode)
{
    sendEngineInitReturnCode(retCode);
}

void EngineServer::onEngineBfeEnableFinished(ENGINE_INIT_RET_CODE retCode)
{
    if (retCode == ENGINE_INIT_SUCCESS)
    {
        onEngineInitFinished(ENGINE_INIT_SUCCESS);
    }
    else
    {
        IPC::ServerCommands::InitFinished cmd(INIT_STATE_BFE_SERVICE_FAILED_TO_START);
        sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
    }
}

void EngineServer::sendFirewallStateChanged(bool isEnabled)
{
    IPC::ServerCommands::FirewallStateChanged cmd;
    cmd.isFirewallEnabled_ = isEnabled;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineFirewallStateChanged(bool isEnabled)
{
    sendFirewallStateChanged(isEnabled);
}

void EngineServer::onEngineLoginFinished(bool isLoginFromSavedSettings, const QString &authHash, const types::PortMap &portMap)
{
    IPC::ServerCommands::LoginFinished cmd(isLoginFromSavedSettings, portMap, authHash);
    qCDebugMultiline(LOG_IPC) << "Engine Settings Changed -- Updating client: " << QString::fromStdString(cmd.getDebugString());
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineLoginError(LOGIN_RET retCode, const QString &errorMessage)
{
    IPC::ServerCommands::LoginError cmd(retCode, errorMessage);
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineLoginMessage(LOGIN_MESSAGE msg)
{
    IPC::ServerCommands::LoginStepMessage cmd(msg);
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineSessionDeleted()
{
    IPC::ServerCommands::SessionDeleted cmd;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineUpdateSessionStatus(const types::SessionStatus &sessionStatus)
{
    IPC::ServerCommands::SessionStatusUpdated cmd(sessionStatus);
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, false);
}

void EngineServer::onEngineNotificationsUpdated(const QVector<types::Notification> &notifications)
{
    IPC::ServerCommands::NotificationsUpdated cmd(notifications);
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, false);
}

void EngineServer::onEngineCheckUpdateUpdated(const types::CheckUpdate &checkUpdate)
{
    IPC::ServerCommands::CheckUpdateInfoUpdated cmd(checkUpdate);
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineUpdateVersionChanged(uint progressPercent, const UPDATE_VERSION_STATE &state, const UPDATE_VERSION_ERROR &error)
{
    IPC::ServerCommands::UpdateVersionChanged cmd;
    cmd.progressPercent = progressPercent;
    cmd.state = state;
    cmd.error = error;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, false);
}

void EngineServer::onEngineMyIpUpdated(const QString &ip, bool /*success*/, bool isDisconnected)
{
    IPC::ServerCommands::MyIpUpdated cmd(ip, isDisconnected);
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, false);
}

void EngineServer::sendConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &locationId)
{
    IPC::ServerCommands::ConnectStateChanged cmd;
    cmd.connectState_.connectState = state;
    cmd.connectState_.disconnectReason = reason;
    cmd.connectState_.connectError = err;
    cmd.connectState_.location = locationId;

    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &locationId)
{
    sendConnectStateChanged(state, reason, err, locationId);
}

void EngineServer::onEngineStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes)
{
    IPC::ServerCommands::StatisticsUpdated cmd;
    cmd.bytesIn_ = bytesIn;
    cmd.bytesOut_ = bytesOut;
    cmd.isTotalBytes_ = isTotalBytes;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, false);
}

void EngineServer::onEngineProtocolPortChanged(const types::ProtocolType &protocol, const uint port)
{
    IPC::ServerCommands::ProtocolPortChanged cmd;
    cmd.protocol_ = protocol;
    cmd.port_ = port;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineEmergencyConnected()
{
    IPC::ServerCommands::EmergencyConnectStateChanged cmd;
    cmd.emergencyConnectState_.connectState = CONNECT_STATE_CONNECTED;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineEmergencyDisconnected()
{
    IPC::ServerCommands::EmergencyConnectStateChanged cmd;
    cmd.emergencyConnectState_.connectState = CONNECT_STATE_DISCONNECTED;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineEmergencyConnectError(CONNECT_ERROR err)
{
    IPC::ServerCommands::EmergencyConnectStateChanged cmd;
    cmd.emergencyConnectState_.connectState = CONNECT_STATE_DISCONNECTED;
    cmd.emergencyConnectState_.disconnectReason = DISCONNECTED_WITH_ERROR;
    cmd.emergencyConnectState_.connectError = err;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineTestTunnelResult(bool bSuccess)
{
    IPC::ServerCommands::TestTunnelResult cmd;
    cmd.success_ = bSuccess;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineLostConnectionToHelper()
{
    IPC::ServerCommands::LostConnectionToHelper cmd;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineProxySharingStateChanged(bool bEnabled, PROXY_SHARING_TYPE proxySharingType)
{
    IPC::ServerCommands::ProxySharingInfoChanged cmd;
    cmd.proxySharingInfo_.isEnabled = bEnabled;
    if (bEnabled)
    {
        cmd.proxySharingInfo_.mode = proxySharingType;
        cmd.proxySharingInfo_.address = engine_->getProxySharingAddress();

    }
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineWifiSharingStateChanged(bool bEnabled, const QString &ssid)
{
    IPC::ServerCommands::WifiSharingInfoChanged cmd;
    cmd.wifiSharingInfo_.isEnabled = bEnabled;
    if (bEnabled)
    {
        cmd.wifiSharingInfo_.ssid = ssid;
    }
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineConnectedWifiUsersCountChanged(int usersCount)
{
    IPC::ServerCommands::WifiSharingInfoChanged cmd;
    cmd.wifiSharingInfo_.usersCount = usersCount;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineConnectedProxyUsersCountChanged(int usersCount)
{
    IPC::ServerCommands::ProxySharingInfoChanged cmd;
    cmd.proxySharingInfo_.usersCount = usersCount;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineSignOutFinished()
{
    IPC::ServerCommands::SignOutFinished cmd;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineGotoCustomOvpnConfigModeFinished()
{
    IPC::ServerCommands::CustomOvpnConfigModeInitFinished cmd;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineNetworkChanged(types::NetworkInterface networkInterface)
{
    IPC::ServerCommands::NetworkChanged cmd(networkInterface);
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineDetectionCpuUsageAfterConnected(QStringList list)
{
    IPC::ServerCommands::HighCpuUsage cmd;
    cmd.processes_ = list;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineRequestUsername()
{
    IPC::ServerCommands::RequestCredentialsForOvpnConfig cmd;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineRequestPassword()
{
    onEngineRequestUsername();
}

void EngineServer::onEngineInternetConnectivityChanged(bool connectivity)
{
    IPC::ServerCommands::InternetConnectivityChanged cmd;
    cmd.connectivity_ = connectivity;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineSendDebugLogFinished(bool bSuccess)
{
    IPC::ServerCommands::DebugLogResult cmd;
    cmd.success_ = bSuccess;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineConfirmEmailFinished(bool bSuccess)
{
    IPC::ServerCommands::ConfirmEmailResult cmd;
    cmd.success_ = bSuccess;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineWebSessionToken(WEB_SESSION_PURPOSE purpose, const QString &token)
{
    IPC::ServerCommands::WebSessionToken cmd(purpose, token);
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineLocationsModelItemsUpdated(const LocationID &bestLocation,  const QString &staticIpDeviceName, QSharedPointer<QVector<types::LocationItem> > items)
{
    IPC::ServerCommands::LocationsUpdated cmd;
    cmd.bestLocation_ = bestLocation;
    cmd.staticIpDeviceName_ = staticIpDeviceName;
    cmd.locations_ = *items;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, false);
}

void EngineServer::onEngineLocationsModelBestLocationUpdated(const LocationID &bestLocation)
{
    IPC::ServerCommands::BestLocationUpdated cmd;
    cmd.bestLocation_ = bestLocation;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, false);
}

void EngineServer::onEngineLocationsModelCustomConfigItemsUpdated(QSharedPointer<QVector<types::LocationItem> > items)
{
    IPC::ServerCommands::CustomConfigLocationsUpdated cmd;
    cmd.locations_ = *items;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, false);
}

void EngineServer::onEngineLocationsModelPingChangedChanged(const LocationID &id, PingTime timeMs)
{
    IPC::ServerCommands::LocationSpeedChanged cmd;
    cmd.id_ = id;
    cmd.pingTime_ = timeMs;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, false);
}

void EngineServer::onMacAddrSpoofingChanged(const types::MacAddrSpoofing &macAddrSpoofing)
{
    curEngineSettings_.setMacAddrSpoofing(macAddrSpoofing);
    IPC::ServerCommands::EngineSettingsChanged cmd(curEngineSettings_);
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEngineSendUserWarning(USER_WARNING_TYPE userWarningType)
{
    IPC::ServerCommands::UserWarning cmd;
    cmd.type_ = userWarningType;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEnginePacketSizeChanged(bool isAuto, int mtu)
{
    types::PacketSize packetSize;
    packetSize.isAutomatic = isAuto;
    packetSize.mtu = mtu;

    curEngineSettings_.setPacketSize(packetSize);

    IPC::ServerCommands::EngineSettingsChanged cmd(curEngineSettings_);
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onEnginePacketSizeDetectionStateChanged(bool on, bool isError)
{
    IPC::ServerCommands::PacketSizeDetectionState cmd;
    cmd.on_ = on;
    cmd.isError_ = isError;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::onHostsFileBecameWritable()
{
    IPC::ServerCommands::HostsFileBecameWritable cmd;
    sendCmdToAllAuthorizedAndGetStateClients(&cmd, true);
}

void EngineServer::sendCmdToAllAuthorizedAndGetStateClients(IPC::Command *cmd, bool bWithLog)
{
    if (bWithLog) {
        qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd->getDebugString());
    }

    Q_EMIT emitCommand(cmd);

    ///sendCmdToAllAuthorizedAndGetStateClientsOfType(cmd, bWithLog, ProtoTypes::CLIENT_ID_GUI, &bLogged);
    ///sendCmdToAllAuthorizedAndGetStateClientsOfType(cmd, bWithLog, ProtoTypes::CLIENT_ID_CLI, &bLogged);

}
