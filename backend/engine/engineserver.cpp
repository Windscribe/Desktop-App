#include "engineserver.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "ipc/server.h"
#include "ipc/protobufcommand.h"
#include "engine/openvpnversioncontroller.h"
#include <QDateTime>

#ifdef Q_OS_WIN
    #include "engine/taputils/tapinstall_win.h"
#endif

EngineServer::EngineServer(QObject *parent) : QObject(parent)
  , server_(NULL)
  , engine_(NULL)
  , threadEngine_(NULL)
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
    if (command->getStringId() == IPCClientCommands::Init::descriptor()->full_name())
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
            connect(threadEngine_, SIGNAL(started()), engine_, SLOT(init()));
            connect(engine_, SIGNAL(cleanupFinished()), threadEngine_, SLOT(quit()));
            connect(threadEngine_, SIGNAL(finished()), SLOT(onEngineCleanupFinished()));

            connect(engine_, SIGNAL(initFinished(ENGINE_INIT_RET_CODE)), SLOT(onEngineInitFinished(ENGINE_INIT_RET_CODE)));
            connect(engine_, SIGNAL(bfeEnableFinished(ENGINE_INIT_RET_CODE)), SLOT(onEngineBfeEnableFinished(ENGINE_INIT_RET_CODE)));
            connect(engine_, SIGNAL(firewallStateChanged(bool)), SLOT(onEngineFirewallStateChanged(bool)));
            connect(engine_, SIGNAL(loginFinished(bool, QString, apiinfo::PortMap)), SLOT(onEngineLoginFinished(bool, QString, apiinfo::PortMap)));
            connect(engine_, SIGNAL(loginError(LOGIN_RET)), SLOT(onEngineLoginError(LOGIN_RET)));
            connect(engine_, SIGNAL(loginStepMessage(LOGIN_MESSAGE)), SLOT(onEngineLoginMessage(LOGIN_MESSAGE)));
            connect(engine_, SIGNAL(notificationsUpdated(QVector<apiinfo::Notification>)), SLOT(onEngineNotificationsUpdated(QVector<apiinfo::Notification>)));
            connect(engine_, SIGNAL(checkUpdateUpdated(bool,QString,ProtoTypes::UpdateChannel,int,QString,bool)), SLOT(onEngineCheckUpdateUpdated(bool,QString,ProtoTypes::UpdateChannel,int,QString,bool)));
            connect(engine_, SIGNAL(updateVersionChanged(uint, ProtoTypes::UpdateVersionState, ProtoTypes::UpdateVersionError)), SLOT(onEngineUpdateVersionChanged(uint, ProtoTypes::UpdateVersionState, ProtoTypes::UpdateVersionError)));
            connect(engine_, SIGNAL(myIpUpdated(QString,bool,bool)), SLOT(onEngineMyIpUpdated(QString,bool,bool)));
            connect(engine_, SIGNAL(sessionStatusUpdated(apiinfo::SessionStatus)), SLOT(onEngineUpdateSessionStatus(apiinfo::SessionStatus)));
            connect(engine_, SIGNAL(sessionDeleted()), SLOT(onEngineSessionDeleted()));
            connect(engine_->getConnectStateController(), SIGNAL(stateChanged(CONNECT_STATE, DISCONNECT_REASON, ProtoTypes::ConnectError, LocationID)),
                    SLOT(onEngineConnectStateChanged(CONNECT_STATE, DISCONNECT_REASON, ProtoTypes::ConnectError, LocationID)));
            connect(engine_, SIGNAL(protocolPortChanged(ProtoTypes::Protocol, uint)), SLOT(onEngineProtocolPortChanged(ProtoTypes::Protocol, uint)));
            connect(engine_, SIGNAL(statisticsUpdated(quint64,quint64, bool)), SLOT(onEngineStatisticsUpdated(quint64,quint64, bool)));
            connect(engine_, SIGNAL(emergencyConnected()), SLOT(onEngineEmergencyConnected()));
            connect(engine_, SIGNAL(emergencyDisconnected()), SLOT(onEngineEmergencyDisconnected()));
            connect(engine_, SIGNAL(emergencyConnectError(ProtoTypes::ConnectError)), SLOT(onEngineEmergencyConnectError(ProtoTypes::ConnectError)));
            connect(engine_, SIGNAL(testTunnelResult(bool)), SLOT(onEngineTestTunnelResult(bool)));
            connect(engine_, SIGNAL(lostConnectionToHelper()), SLOT(onEngineLostConnectionToHelper()));
            connect(engine_, SIGNAL(proxySharingStateChanged(bool, PROXY_SHARING_TYPE)), SLOT(onEngineProxySharingStateChanged(bool, PROXY_SHARING_TYPE)));
            connect(engine_, SIGNAL(wifiSharingStateChanged(bool, QString)), SLOT(onEngineWifiSharingStateChanged(bool, QString)));
            connect(engine_, SIGNAL(vpnSharingConnectedWifiUsersCountChanged(int)), SLOT(onEngineConnectedWifiUsersCountChanged(int)));
            connect(engine_, SIGNAL(vpnSharingConnectedProxyUsersCountChanged(int)), SLOT(onEngineConnectedProxyUsersCountChanged(int)));
            connect(engine_, SIGNAL(signOutFinished()), SLOT(onEngineSignOutFinished()));
            connect(engine_, SIGNAL(gotoCustomOvpnConfigModeFinished()), SLOT(onEngineGotoCustomOvpnConfigModeFinished()));
            connect(engine_, SIGNAL(detectionCpuUsageAfterConnected(QStringList)), SLOT(onEngineDetectionCpuUsageAfterConnected(QStringList)));
            connect(engine_, SIGNAL(requestUsername()), SLOT(onEngineRequestUsername()));
            connect(engine_, SIGNAL(requestPassword()), SLOT(onEngineRequestPassword()));
            connect(engine_, SIGNAL(networkChanged(ProtoTypes::NetworkInterface)), SLOT(onEngineNetworkChanged(ProtoTypes::NetworkInterface)));
            connect(engine_, SIGNAL(confirmEmailFinished(bool)), SLOT(onEngineConfirmEmailFinished(bool)));
            connect(engine_, SIGNAL(sendDebugLogFinished(bool)), SLOT(onEngineSendDebugLogFinished(bool)));
            connect(engine_, SIGNAL(webSessionToken(QString)), SLOT(onEngineWebSessionToken(QString)));
            connect(engine_, SIGNAL(macAddrSpoofingChanged(ProtoTypes::MacAddrSpoofing)), SLOT(onMacAddrSpoofingChanged(ProtoTypes::MacAddrSpoofing)));
            connect(engine_, SIGNAL(sendUserWarning(ProtoTypes::UserWarningType)), SLOT(onEngineSendUserWarning(ProtoTypes::UserWarningType)));
            connect(engine_, SIGNAL(internetConnectivityChanged(bool)), SLOT(onEngineInternetConnectivityChanged(bool)));
            connect(engine_, SIGNAL(packetSizeChanged(bool, int)), SLOT(onEnginePacketSizeChanged(bool, int)));
            connect(engine_, SIGNAL(packetSizeDetectionStateChanged(bool,bool)), SLOT(onEnginePacketSizeDetectionStateChanged(bool,bool)));
            connect(engine_, SIGNAL(hostsFileBecameWritable()), SLOT(onHostsFileBecameWritable()));
            threadEngine_->start(QThread::LowPriority);
        }
        else
        {
            qCDebug(LOG_IPC) << "Engine already built";

            IPC::ProtobufCommand<IPCServerCommands::InitFinished> cmd;
            cmd.getProtoObj().set_init_state(ProtoTypes::INIT_SUCCESS);
            *cmd.getProtoObj().mutable_engine_settings() = curEngineSettings_.getProtoBufEngineSettings();
            const QStringList vers = OpenVpnVersionController::instance().getAvailableOpenVpnVersions();
            for (const QString &openVpnVer : vers)
            {
                cmd.getProtoObj().add_available_openvpn_versions(openVpnVer.toStdString());
            }
            cmd.getProtoObj().set_is_wifi_sharing_supported(engine_->isWifiSharingSupported());
            cmd.getProtoObj().set_is_saved_api_settings_exists(engine_->isApiSavedSettingsExists());
            cmd.getProtoObj().set_auth_hash(engine_->getAuthHash().toStdString());

            sendCmdToAllAuthorizedAndGetStateClientsOfType(cmd, true, ProtoTypes::CLIENT_ID_CLI);
        }

        return true;
    }
    else if (command->getStringId() == IPCClientCommands::EnableBfe_win::descriptor()->full_name())
    {
        engine_->enableBFE_win();
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::Cleanup::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCClientCommands::Cleanup> *cleanupCmd = static_cast<IPC::ProtobufCommand<IPCClientCommands::Cleanup> *>(command);

        if (engine_ == NULL)
        {
            Q_EMIT finished();
        }
        else
        {
            engine_->cleanup(cleanupCmd->getProtoObj().is_exit_with_restart(), cleanupCmd->getProtoObj().is_firewall_checked(),
                             cleanupCmd->getProtoObj().is_firewall_always_on(), cleanupCmd->getProtoObj().is_launch_on_start());
        }
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::Firewall::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCClientCommands::Firewall> *firewallCmd = static_cast<IPC::ProtobufCommand<IPCClientCommands::Firewall> *>(command);
        if (firewallCmd->getProtoObj().is_enable())
        {
            engine_->firewallOn();
        }
        else
        {
            engine_->firewallOff();
        }
        return true;
    }

    else if (command->getStringId() == IPCClientCommands::Login::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCClientCommands::Login> *loginCmd = static_cast<IPC::ProtobufCommand<IPCClientCommands::Login> *>(command);

        // hide password for logging
        IPC::ProtobufCommand<IPCClientCommands::Login> loggedCmd = *loginCmd;
        loggedCmd.getProtoObj().set_password("*****");
        qCDebugMultiline(LOG_IPC) << QString::fromStdString(loggedCmd.getDebugString());

        // login with last login settings
        if (loginCmd->getProtoObj().use_last_login_settings() == true)
        {
            engine_->loginWithLastLoginSettings();
        }
        // login with auth hash
        else if (!loginCmd->getProtoObj().auth_hash().empty())
        {
            engine_->loginWithAuthHash(QString::fromStdString(loginCmd->getProtoObj().auth_hash()));
        }
        // login with username and password
        else if (!loginCmd->getProtoObj().username().empty() && !loginCmd->getProtoObj().password().empty())
        {
            engine_->loginWithUsernameAndPassword(QString::fromStdString(loginCmd->getProtoObj().username()),
                                                  QString::fromStdString(loginCmd->getProtoObj().password()),
                                                  QString::fromStdString(loginCmd->getProtoObj().code2fa()));
        }
        else
        {
            Q_ASSERT(false);
            return false;
        }
    }
    else if (command->getStringId() == IPCClientCommands::ApplicationActivated::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCClientCommands::ApplicationActivated> *appActivatedCmd = static_cast<IPC::ProtobufCommand<IPCClientCommands::ApplicationActivated> *>(command);
        if (appActivatedCmd->getProtoObj().is_activated())
        {
            engine_->applicationActivated();
        }
        else
        {
            engine_->applicationDeactivated();
        }
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::Connect::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCClientCommands::Connect> *connectCmd = static_cast<IPC::ProtobufCommand<IPCClientCommands::Connect> *>(command);
        engine_->connectClick(LocationID::createFromProtoBuf(connectCmd->getProtoObj().locationdid()));
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::Disconnect::descriptor()->full_name())
    {
        engine_->disconnectClick();
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::StartProxySharing::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCClientCommands::StartProxySharing> *cmd = static_cast<IPC::ProtobufCommand<IPCClientCommands::StartProxySharing> *>(command);

        PROXY_SHARING_TYPE t = (cmd->getProtoObj().sharing_mode() == ProtoTypes::PROXY_SHARING_HTTP) ? PROXY_SHARING_HTTP : PROXY_SHARING_SOCKS;
        engine_->startProxySharing(t);
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::StopProxySharing::descriptor()->full_name())
    {
        engine_->stopProxySharing();
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::StartWifiSharing::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCClientCommands::StartWifiSharing> *cmd = static_cast<IPC::ProtobufCommand<IPCClientCommands::StartWifiSharing> *>(command);
        engine_->startWifiSharing(QString::fromStdString(cmd->getProtoObj().ssid()), QString::fromStdString(cmd->getProtoObj().password()));
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::StopWifiSharing::descriptor()->full_name())
    {
        engine_->stopWifiSharing();
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::EmergencyConnect::descriptor()->full_name())
    {
        engine_->emergencyConnectClick();
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::EmergencyDisconnect::descriptor()->full_name())
    {
        engine_->emergencyDisconnectClick();
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::SignOut::descriptor()->full_name())
    {
        engine_->signOut();
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::SendDebugLog::descriptor()->full_name())
    {
        engine_->sendDebugLog();
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::EditAccountDetails::descriptor()->full_name())
    {
        engine_->editAccountDetails();
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::SendConfirmEmail::descriptor()->full_name())
    {
        engine_->sendConfirmEmail();
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::RecordInstall::descriptor()->full_name())
    {
        engine_->recordInstall();
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::GetIpv6StateInOS::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::Ipv6StateInOS> cmd;
        cmd.getProtoObj().set_is_enabled(engine_->IPv6StateInOS());
        sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::SetIpv6StateInOS::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCClientCommands::SetIpv6StateInOS> *cmd = static_cast<IPC::ProtobufCommand<IPCClientCommands::SetIpv6StateInOS> *>(command);
        engine_->setIPv6EnabledInOS(cmd->getProtoObj().is_enabled());
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::SplitTunneling::descriptor()->full_name())
    {
        // qDebug() << "Received split tunneling command from GUI";
        IPC::ProtobufCommand<IPCClientCommands::SplitTunneling> *cmd = static_cast<IPC::ProtobufCommand<IPCClientCommands::SplitTunneling> *>(command);

        bool isActive = cmd->getProtoObj().split_tunneling().settings().active();
        bool isExclude = cmd->getProtoObj().split_tunneling().settings().mode() == ProtoTypes::SPLIT_TUNNELING_MODE_EXCLUDE;

        QStringList files;
        for (int i = 0; i < cmd->getProtoObj().split_tunneling().apps_size(); ++i)
        {
            if (cmd->getProtoObj().split_tunneling().apps(i).active())
            {
                files << QString::fromStdString(cmd->getProtoObj().split_tunneling().apps(i).full_name());
            }
        }

        QStringList ips;
        QStringList hosts;
        for (int i = 0; i < cmd->getProtoObj().split_tunneling().network_routes_size(); ++i)
        {
            if (cmd->getProtoObj().split_tunneling().network_routes(i).type() == ProtoTypes::SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP)
            {
                ips << QString::fromStdString(cmd->getProtoObj().split_tunneling().network_routes(i).name());
            }
            else if (cmd->getProtoObj().split_tunneling().network_routes(i).type() == ProtoTypes::SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_HOSTNAME)
            {
                hosts << QString::fromStdString(cmd->getProtoObj().split_tunneling().network_routes(i).name());
            }
        }

        engine_->setSplitTunnelingSettings(isActive, isExclude, files, ips, hosts);
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::GotoCustomOvpnConfigMode::descriptor()->full_name())
    {
        engine_->gotoCustomOvpnConfigMode();
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::GetSettings::descriptor()->full_name())
    {
        //IPC::Command *cmd = curEngineSettings_.transformToProtoBufCommand(command->getCmdUid());
        //*outCommand = cmd;
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::SetSettings::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCClientCommands::SetSettings> *setSettingsCmd = static_cast<IPC::ProtobufCommand<IPCClientCommands::SetSettings> *>(command);

        if (!curEngineSettings_.isEqual(setSettingsCmd->getProtoObj().enginesettings()))
        {
            curEngineSettings_ = EngineSettings(setSettingsCmd->getProtoObj().enginesettings());
            if (engine_)
            {
                engine_->setSettings(curEngineSettings_);
            }
            curEngineSettings_.saveToSettings();

            //todo ?
            //Q_EMIT engineSettingsChanged(curEngineSettings_, connection);
        }
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::SetBlockConnect::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCClientCommands::SetBlockConnect> *blockConnectCmd = static_cast<IPC::ProtobufCommand<IPCClientCommands::SetBlockConnect> *>(command);
        if (engine_->isBlockConnect() != blockConnectCmd->getProtoObj().is_block_connect())
        {
            qCDebugMultiline(LOG_IPC) << QString::fromStdString(command->getDebugString());
            engine_->setBlockConnect(blockConnectCmd->getProtoObj().is_block_connect());
        }
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::ClearCredentials::descriptor()->full_name())
    {
        engine_->clearCredentials();
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::SpeedRating::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCClientCommands::SpeedRating> *cmd = static_cast<IPC::ProtobufCommand<IPCClientCommands::SpeedRating> *>(command);
        engine_->speedRating(cmd->getProtoObj().rating(), QString::fromStdString(cmd->getProtoObj().local_external_ip()));
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::ContinueWithCredentialsForOvpnConfig::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCClientCommands::ContinueWithCredentialsForOvpnConfig> *cmd = static_cast<IPC::ProtobufCommand<IPCClientCommands::ContinueWithCredentialsForOvpnConfig> *>(command);
        engine_->continueWithUsernameAndPassword(QString::fromStdString(cmd->getProtoObj().username()),
                                                 QString::fromStdString(cmd->getProtoObj().password()), cmd->getProtoObj().is_save());
        return true;
    }
    else if (command->getStringId() == IPCClientCommands::ForceCliStateUpdate::descriptor()->full_name())
    {
        sendFirewallStateChanged(engine_->isFirewallEnabled()); // this must happen before others

        engine_->forceUpdateServerLocations();

        IConnectStateController *eCSC = engine_->getConnectStateController();
        sendConnectStateChanged(eCSC->currentState(), eCSC->disconnectReason(), eCSC->connectionError(), eCSC->locationId());

        return true;
    }
    else if (command->getStringId() == IPCClientCommands::DetectPacketSize::descriptor()->full_name())
    {
        engine_->detectAppropriatePacketSize();
    }
    else if (command->getStringId() == IPCClientCommands::UpdateVersion::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCClientCommands::UpdateVersion> *cmd = static_cast<IPC::ProtobufCommand<IPCClientCommands::UpdateVersion> *>(command);
        if (cmd->getProtoObj().cancel_download())
        {
            engine_->stopUpdateVersion();
        }
        else
        {
            engine_->updateVersion(cmd->getProtoObj().hwnd());
        }
    }
    else if (command->getStringId() == IPCClientCommands::UpdateWindowInfo::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCClientCommands::UpdateWindowInfo> *cmd = static_cast<IPC::ProtobufCommand<IPCClientCommands::UpdateWindowInfo> *>(command);
        engine_->updateWindowInfo(cmd->getProtoObj().window_center_x(), cmd->getProtoObj().window_center_y());
    }
    else if (command->getStringId() == IPCClientCommands::MakeHostsWritableWin::descriptor()->full_name()) {
#ifdef Q_OS_WIN
        engine_->makeHostsFileWritableWin();
#endif
    }

    return false;
}

void EngineServer::sendEngineInitReturnCode(ENGINE_INIT_RET_CODE retCode)
{
    IPC::ProtobufCommand<IPCServerCommands::InitFinished> cmd;

    if (retCode == ENGINE_INIT_SUCCESS)
    {
        connect(engine_->getLocationsModel(), SIGNAL(locationsUpdated(LocationID, QString, QSharedPointer<QVector<locationsmodel::LocationItem> >)),
                SLOT(onEngineLocationsModelItemsUpdated(LocationID, QString, QSharedPointer<QVector<locationsmodel::LocationItem> >)));
        connect(engine_->getLocationsModel(), SIGNAL(locationsUpdatedCliOnly(LocationID, QSharedPointer<QVector<locationsmodel::LocationItem> >)),
                SLOT(onEngineLocationsModelItemsUpdatedCliOnly(LocationID, QSharedPointer<QVector<locationsmodel::LocationItem> >)));
        connect(engine_->getLocationsModel(), SIGNAL(bestLocationUpdated(LocationID)),
                SLOT(onEngineLocationsModelBestLocationUpdated(LocationID)));
        connect(engine_->getLocationsModel(), SIGNAL(customConfigsLocationsUpdated(QSharedPointer<QVector<locationsmodel::LocationItem> >)),
                SLOT(onEngineLocationsModelCustomConfigItemsUpdated(QSharedPointer<QVector<locationsmodel::LocationItem> >)));
        connect(engine_->getLocationsModel(), SIGNAL(locationPingTimeChanged(LocationID,locationsmodel::PingTime)),
                SLOT(onEngineLocationsModelPingChangedChanged(LocationID,locationsmodel::PingTime)));

        cmd.getProtoObj().set_init_state(ProtoTypes::INIT_SUCCESS);
        *cmd.getProtoObj().mutable_engine_settings() = curEngineSettings_.getProtoBufEngineSettings();

        const QStringList vers = OpenVpnVersionController::instance().getAvailableOpenVpnVersions();
        for (const QString &openVpnVer : vers)
        {
            cmd.getProtoObj().add_available_openvpn_versions(openVpnVer.toStdString());
        }

        cmd.getProtoObj().set_is_wifi_sharing_supported(engine_->isWifiSharingSupported());

        cmd.getProtoObj().set_is_saved_api_settings_exists(engine_->isApiSavedSettingsExists());
        cmd.getProtoObj().set_auth_hash(engine_->getAuthHash().toStdString());

        sendCmdToAllAuthorizedAndGetStateClients(cmd, true);

        engine_->updateCurrentInternetConnectivity();
    }
    else if (retCode == ENGINE_INIT_HELPER_FAILED)
    {
        cmd.getProtoObj().set_init_state(ProtoTypes::INIT_HELPER_FAILED);
        sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
    }
    else if (retCode == ENGINE_INIT_BFE_SERVICE_FAILED)
    {
        cmd.getProtoObj().set_init_state(ProtoTypes::INIT_BFE_SERVICE_NOT_STARTED);
        sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
    }
    else if (retCode == ENGINE_INIT_HELPER_USER_CANCELED)
    {
        cmd.getProtoObj().set_init_state(ProtoTypes::INIT_HELPER_USER_CANCELED);
        sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
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

void EngineServer::onConnectionCommandCallback(IPC::Command *command, IPC::IConnection *connection)
{
    if ((command->getStringId() != IPCClientCommands::Login::descriptor()->full_name()) &&
        (command->getStringId() != IPCClientCommands::SetBlockConnect::descriptor()->full_name()))
    {
        // The SetBlockConnect command is received every minute.  handleCommand will log it if the value
        // has changed, so that we don't flood the log with this entry when the app is up for an
        // extended period of time.
        qCDebugMultiline(LOG_IPC) << QString::fromStdString(command->getDebugString());
    }

    auto itClient = connections_.find(connection);
    if (itClient != connections_.end())
    {
        // check if the client is made authorization
        ClientConnectionDescr &clientConnectionDescr = connections_[connection];
        if (clientConnectionDescr.bClientAuthReceived_)
        {
            handleCommand(command);
        }
        else
        {
            // wait for command ClientAuth for authorization of client
            if (command->getStringId() == IPCClientCommands::ClientAuth::descriptor()->full_name())
            {
                IPC::ProtobufCommand<IPCClientCommands::ClientAuth> *cmdClientAuth = static_cast<IPC::ProtobufCommand<IPCClientCommands::ClientAuth> *>(command);

                clientConnectionDescr.bClientAuthReceived_ = true;
                clientConnectionDescr.clientId_ = cmdClientAuth->getProtoObj().client_id();
                clientConnectionDescr.protocolVersion_ = cmdClientAuth->getProtoObj().protocol_version();
                clientConnectionDescr.pid_ = cmdClientAuth->getProtoObj().pid();
                clientConnectionDescr.name_ = QString::fromStdString(cmdClientAuth->getProtoObj().name());
                clientConnectionDescr.latestCommandTimeMs_ = QDateTime::currentMSecsSinceEpoch();

                IPC::ProtobufCommand<IPCServerCommands::AuthReply> cmdReply;
                connection->sendCommand(cmdReply);
            }
            // all other commands - close connection
            else
            {
                dynamic_cast<QObject*>(connection)->disconnect();
                connections_.remove(connection);
                SAFE_DELETE(connection);
            }
        }
    }
    else
    {
        // TODO: re-enable this assert?
		// Q_ASSERT(false);
    }


    delete command;
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
    IPC::ProtobufCommand<IPCServerCommands::CleanupFinished> cmd;
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
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
        IPC::ProtobufCommand<IPCServerCommands::InitFinished> cmd;
        cmd.getProtoObj().set_init_state(ProtoTypes::INIT_BFE_SERVICE_FAILED_TO_START);
        sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
    }
}

void EngineServer::sendFirewallStateChanged(bool isEnabled)
{
    IPC::ProtobufCommand<IPCServerCommands::FirewallStateChanged> cmd;
    cmd.getProtoObj().set_is_firewall_enabled(isEnabled);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineFirewallStateChanged(bool isEnabled)
{
    sendFirewallStateChanged(isEnabled);
}

void EngineServer::onEngineLoginFinished(bool isLoginFromSavedSettings, const QString &authHash, const apiinfo::PortMap &portMap)
{
    IPC::ProtobufCommand<IPCServerCommands::LoginFinished> cmd;
    cmd.getProtoObj().set_is_login_from_saved_settings(isLoginFromSavedSettings);
    cmd.getProtoObj().set_auth_hash(authHash.toStdString());
    qCDebugMultiline(LOG_IPC) << "Engine Settings Changed -- Updating client: " << QString::fromStdString(cmd.getDebugString());

    ProtoTypes::ArrayPortMap arrPortMap;
    for (int i = 0; i < portMap.getPortItemCount(); ++i)
    {
        const apiinfo::PortItem *portItem = portMap.getPortItemByIndex(i);
        ProtoTypes::PortMapItem pmi;
        pmi.set_protocol(portItem->protocol.convertToProtobuf());
        pmi.set_heading(portItem->heading.toStdString());
        pmi.set_use(portItem->use.toStdString());

        for (int p = 0; p < portItem->ports.count(); ++p)
        {
            pmi.add_ports(portItem->ports[p]);
        }
        *arrPortMap.add_port_map_item() = pmi;
    }
    *cmd.getProtoObj().mutable_array_port_map() = arrPortMap;

    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineLoginError(LOGIN_RET retCode)
{
    IPC::ProtobufCommand<IPCServerCommands::LoginError> cmd;
    cmd.getProtoObj().set_error(loginRetToProtobuf(retCode));
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineLoginMessage(LOGIN_MESSAGE msg)
{
    IPC::ProtobufCommand<IPCServerCommands::LoginStepMessage> cmd;
    if (msg == LOGIN_MESSAGE_NONE)
    {
        cmd.getProtoObj().set_message(ProtoTypes::LOGIN_MESSAGE_NONE);
    }
    else if (msg == LOGIN_MESSAGE_TRYING_BACKUP1)
    {
        cmd.getProtoObj().set_message(ProtoTypes::LOGIN_MESSAGE_TRYING_BACKUP1);
    }
    else if (msg == LOGIN_MESSAGE_TRYING_BACKUP2)
    {
        cmd.getProtoObj().set_message(ProtoTypes::LOGIN_MESSAGE_TRYING_BACKUP2);
    }
    else
    {
        Q_ASSERT(false);
    }
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineSessionDeleted()
{
    IPC::ProtobufCommand<IPCServerCommands::SessionDeleted> cmd;
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineUpdateSessionStatus(const apiinfo::SessionStatus &sessionStatus)
{
    IPC::ProtobufCommand<IPCServerCommands::SessionStatusUpdated> cmd;
    *cmd.getProtoObj().mutable_session_status() = sessionStatus.getProtoBuf();
    sendCmdToAllAuthorizedAndGetStateClients(cmd, false);
}

void EngineServer::onEngineNotificationsUpdated(const QVector<apiinfo::Notification> &notifications)
{
    IPC::ProtobufCommand<IPCServerCommands::NotificationsUpdated> cmd;

    for (const apiinfo::Notification &n : notifications)
    {
        *cmd.getProtoObj().mutable_array_notifications()->add_api_notifications() = n.getProtoBuf();
    }
    sendCmdToAllAuthorizedAndGetStateClients(cmd, false);
}

void EngineServer::onEngineCheckUpdateUpdated(bool available, const QString &version, const ProtoTypes::UpdateChannel updateChannel, int latestBuild, const QString &url, bool supported)
{
    IPC::ProtobufCommand<IPCServerCommands::CheckUpdateInfoUpdated> cmd;
    cmd.getProtoObj().mutable_check_update_info()->set_is_available(available);
    cmd.getProtoObj().mutable_check_update_info()->set_version(version.toStdString());
    cmd.getProtoObj().mutable_check_update_info()->set_update_channel(updateChannel);
    cmd.getProtoObj().mutable_check_update_info()->set_latest_build(latestBuild);
    cmd.getProtoObj().mutable_check_update_info()->set_url(url.toStdString());
    cmd.getProtoObj().mutable_check_update_info()->set_is_supported(supported);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineUpdateVersionChanged(uint progressPercent, const ProtoTypes::UpdateVersionState &state, const ProtoTypes::UpdateVersionError &error)
{
    IPC::ProtobufCommand<IPCServerCommands::UpdateVersionChanged> cmd;
    cmd.getProtoObj().set_progress(progressPercent);
    cmd.getProtoObj().set_state(state);
    cmd.getProtoObj().set_error(error);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, false);
}

void EngineServer::onEngineMyIpUpdated(const QString &ip, bool /*success*/, bool isDisconnected)
{
    // censor user IP for log
    if (isDisconnected)
    {
        int index = ip.lastIndexOf('.');
        QString censoredIp = ip.mid(0,index+1) + "###"; // censor last octet
        IPC::ProtobufCommand<IPCServerCommands::MyIpUpdated> cmdLoggingOnly;
        cmdLoggingOnly.getProtoObj().mutable_my_ip_info()->set_ip(censoredIp.toStdString());
        cmdLoggingOnly.getProtoObj().mutable_my_ip_info()->set_is_disconnected_state(isDisconnected);
        qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmdLoggingOnly.getDebugString());
    }

    IPC::ProtobufCommand<IPCServerCommands::MyIpUpdated> cmd;
    cmd.getProtoObj().mutable_my_ip_info()->set_ip(ip.toStdString());
    cmd.getProtoObj().mutable_my_ip_info()->set_is_disconnected_state(isDisconnected);

    sendCmdToAllAuthorizedAndGetStateClients(cmd, false); // only non-user IP
}

void EngineServer::sendConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, ProtoTypes::ConnectError err, const LocationID &locationId)
{
    IPC::ProtobufCommand<IPCServerCommands::ConnectStateChanged> cmd;

    //ProtoTypes::ConnectState c;
    if (state == CONNECT_STATE_DISCONNECTED)
    {
        cmd.getProtoObj().mutable_connect_state()->set_connect_state_type(ProtoTypes::DISCONNECTED);
    }
    else if (state == CONNECT_STATE_CONNECTED)
    {
        cmd.getProtoObj().mutable_connect_state()->set_connect_state_type(ProtoTypes::CONNECTED);
    }
    else if (state == CONNECT_STATE_CONNECTING)
    {
        cmd.getProtoObj().mutable_connect_state()->set_connect_state_type(ProtoTypes::CONNECTING);
    }
    else if (state == CONNECT_STATE_DISCONNECTING)
    {
        cmd.getProtoObj().mutable_connect_state()->set_connect_state_type(ProtoTypes::DISCONNECTING);
    }
    else
    {
        Q_ASSERT(false);
    }

    if (state == CONNECT_STATE_DISCONNECTED)
    {
        cmd.getProtoObj().mutable_connect_state()->set_disconnect_reason((ProtoTypes::DisconnectReason)reason);
        if (reason == DISCONNECTED_WITH_ERROR)
        {
            cmd.getProtoObj().mutable_connect_state()->set_connect_error(err);
        }
    }
    else if (state == CONNECT_STATE_CONNECTED || state == CONNECT_STATE_CONNECTING)
    {
        if (locationId.isValid())
        {
            *cmd.getProtoObj().mutable_connect_state()->mutable_location() = locationId.toProtobuf();
        }
    }

    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, ProtoTypes::ConnectError err, const LocationID &locationId)
{
    sendConnectStateChanged(state, reason, err, locationId);
}

void EngineServer::onEngineStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes)
{
    IPC::ProtobufCommand<IPCServerCommands::StatisticsUpdated> cmd;
    cmd.getProtoObj().set_bytes_in(bytesIn);
    cmd.getProtoObj().set_bytes_out(bytesOut);
    cmd.getProtoObj().set_is_total_bytes(isTotalBytes);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, false);
}

void EngineServer::onEngineProtocolPortChanged(const ProtoTypes::Protocol &protocol, const uint port)
{
    IPC::ProtobufCommand<IPCServerCommands::ProtocolPortChanged> cmd;
    cmd.getProtoObj().set_protocol(protocol);
    cmd.getProtoObj().set_port(port);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineEmergencyConnected()
{
    IPC::ProtobufCommand<IPCServerCommands::EmergencyConnectStateChanged> cmd;
    cmd.getProtoObj().mutable_emergency_connect_state()->set_connect_state_type(ProtoTypes::CONNECTED);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineEmergencyDisconnected()
{
    IPC::ProtobufCommand<IPCServerCommands::EmergencyConnectStateChanged> cmd;
    cmd.getProtoObj().mutable_emergency_connect_state()->set_connect_state_type(ProtoTypes::DISCONNECTED);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineEmergencyConnectError(ProtoTypes::ConnectError err)
{
    IPC::ProtobufCommand<IPCServerCommands::EmergencyConnectStateChanged> cmd;
    cmd.getProtoObj().mutable_emergency_connect_state()->set_connect_state_type(ProtoTypes::DISCONNECTED);
    cmd.getProtoObj().mutable_emergency_connect_state()->set_disconnect_reason(ProtoTypes::DISCONNECTED_WITH_ERROR);
    cmd.getProtoObj().mutable_emergency_connect_state()->set_connect_error(err);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineTestTunnelResult(bool bSuccess)
{
    IPC::ProtobufCommand<IPCServerCommands::TestTunnelResult> cmd;
    cmd.getProtoObj().set_success(bSuccess);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineLostConnectionToHelper()
{
    IPC::ProtobufCommand<IPCServerCommands::LostConnectionToHelper> cmd;
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineProxySharingStateChanged(bool bEnabled, PROXY_SHARING_TYPE proxySharingType)
{
    IPC::ProtobufCommand<IPCServerCommands::ProxySharingInfoChanged> cmd;

    cmd.getProtoObj().mutable_proxy_sharing_info()->set_is_enabled(bEnabled);
    if (bEnabled)
    {
        cmd.getProtoObj().mutable_proxy_sharing_info()->set_mode(proxySharingType == PROXY_SHARING_HTTP ? ProtoTypes::PROXY_SHARING_HTTP : ProtoTypes::PROXY_SHARING_SOCKS);
        cmd.getProtoObj().mutable_proxy_sharing_info()->set_address(engine_->getProxySharingAddress().toStdString());
    }
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineWifiSharingStateChanged(bool bEnabled, const QString &ssid)
{
    IPC::ProtobufCommand<IPCServerCommands::WifiSharingInfoChanged> cmd;

    cmd.getProtoObj().mutable_wifi_sharing_info()->set_is_enabled(bEnabled);
    if (bEnabled)
    {
        cmd.getProtoObj().mutable_wifi_sharing_info()->set_ssid(ssid.toStdString());
    }

    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineConnectedWifiUsersCountChanged(int usersCount)
{
    IPC::ProtobufCommand<IPCServerCommands::WifiSharingInfoChanged> cmd;
    cmd.getProtoObj().mutable_wifi_sharing_info()->set_users_count(usersCount);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineConnectedProxyUsersCountChanged(int usersCount)
{
    IPC::ProtobufCommand<IPCServerCommands::ProxySharingInfoChanged> cmd;
    cmd.getProtoObj().mutable_proxy_sharing_info()->set_users_count(usersCount);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineSignOutFinished()
{
    IPC::ProtobufCommand<IPCServerCommands::SignOutFinished> cmd;
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineGotoCustomOvpnConfigModeFinished()
{
    IPC::ProtobufCommand<IPCServerCommands::CustomOvpnConfigModeInitFinished> cmd;
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineNetworkChanged(ProtoTypes::NetworkInterface networkInterface)
{
    IPC::ProtobufCommand<IPCServerCommands::NetworkChanged> cmd;
    *cmd.getProtoObj().mutable_network_interface() = networkInterface;
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineDetectionCpuUsageAfterConnected(QStringList list)
{
    IPC::ProtobufCommand<IPCServerCommands::HighCpuUsage> cmd;
    for (const QString &p : qAsConst(list))
    {
        *cmd.getProtoObj().add_processes() = p.toStdString();
    }
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineRequestUsername()
{
    IPC::ProtobufCommand<IPCServerCommands::RequestCredentialsForOvpnConfig> cmd;
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineRequestPassword()
{
    onEngineRequestUsername();
}

void EngineServer::onEngineInternetConnectivityChanged(bool connectivity)
{
    IPC::ProtobufCommand<IPCServerCommands::InternetConnectivityChanged> cmd;
    cmd.getProtoObj().set_connectivity(connectivity);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineSendDebugLogFinished(bool bSuccess)
{
    IPC::ProtobufCommand<IPCServerCommands::DebugLogResult> cmd;
    cmd.getProtoObj().set_success(bSuccess);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineConfirmEmailFinished(bool bSuccess)
{
    IPC::ProtobufCommand<IPCServerCommands::ConfirmEmailResult> cmd;
    cmd.getProtoObj().set_success(bSuccess);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineWebSessionToken(const QString &token)
{
    IPC::ProtobufCommand<IPCServerCommands::WebSessionToken> cmd;
    cmd.getProtoObj().set_temp_session_token(token.toStdString());
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineLocationsModelItemsUpdated(const LocationID &bestLocation,  const QString &staticIpDeviceName, QSharedPointer<QVector<locationsmodel::LocationItem> > items)
{
    IPC::ProtobufCommand<IPCServerCommands::LocationsUpdated> cmd;

    *cmd.getProtoObj().mutable_best_location() = bestLocation.toProtobuf();
    cmd.getProtoObj().set_static_ip_device_name(staticIpDeviceName.toStdString());

    for (const locationsmodel::LocationItem &li : *items)
    {
        ProtoTypes::Location *l = cmd.getProtoObj().mutable_locations()->add_locations();
        li.fillProtobuf(l);
    }

    sendCmdToAllAuthorizedAndGetStateClients(cmd, false);
}

void EngineServer::onEngineLocationsModelItemsUpdatedCliOnly(const LocationID &bestLocation, QSharedPointer<QVector<locationsmodel::LocationItem> > items)
{
    IPC::ProtobufCommand<IPCServerCommands::LocationsUpdated> cmd;
    *cmd.getProtoObj().mutable_best_location() = bestLocation.toProtobuf();

    for (const locationsmodel::LocationItem &li : *items)
    {
        ProtoTypes::Location *l = cmd.getProtoObj().mutable_locations()->add_locations();
        li.fillProtobuf(l);
    }
    sendCmdToAllAuthorizedAndGetStateClientsOfType(cmd, false, ProtoTypes::CLIENT_ID_CLI);
}

void EngineServer::onEngineLocationsModelBestLocationUpdated(const LocationID &bestLocation)
{
    IPC::ProtobufCommand<IPCServerCommands::BestLocationUpdated> cmd;
    *cmd.getProtoObj().mutable_best_location() = bestLocation.toProtobuf();
    sendCmdToAllAuthorizedAndGetStateClients(cmd, false);
}

void EngineServer::onEngineLocationsModelCustomConfigItemsUpdated(QSharedPointer<QVector<locationsmodel::LocationItem> > items)
{
    IPC::ProtobufCommand<IPCServerCommands::CustomConfigLocationsUpdated> cmd;

    for (const locationsmodel::LocationItem &li : *items)
    {
        ProtoTypes::Location *l = cmd.getProtoObj().mutable_locations()->add_locations();
        li.fillProtobuf(l);
    }

    sendCmdToAllAuthorizedAndGetStateClients(cmd, false);
}

void EngineServer::onEngineLocationsModelPingChangedChanged(const LocationID &id, locationsmodel::PingTime timeMs)
{
    IPC::ProtobufCommand<IPCServerCommands::LocationSpeedChanged> cmd;
    *cmd.getProtoObj().mutable_id() = id.toProtobuf();
    cmd.getProtoObj().set_pingtime(timeMs.toInt());
    sendCmdToAllAuthorizedAndGetStateClients(cmd, false);
}

void EngineServer::onMacAddrSpoofingChanged(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing)
{
    // qDebug() << "EngineServer::onMacAddrspoofingChanged";
    curEngineSettings_.setMacAddrSpoofing(macAddrSpoofing);

    IPC::ProtobufCommand<IPCServerCommands::EngineSettingsChanged> cmd;
    *cmd.getProtoObj().mutable_enginesettings() = curEngineSettings_.getProtoBufEngineSettings();
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEngineSendUserWarning(ProtoTypes::UserWarningType userWarningType)
{
    IPC::ProtobufCommand<IPCServerCommands::UserWarning> cmd;
    cmd.getProtoObj().set_type(userWarningType);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onEnginePacketSizeChanged(bool isAuto, int mtu)
{
    ProtoTypes::PacketSize packetSize;
    packetSize.set_is_automatic(isAuto);
    packetSize.set_mtu(mtu);

    curEngineSettings_.setPacketSize(packetSize);

    IPC::ProtobufCommand<IPCServerCommands::EngineSettingsChanged> cmd;
    *cmd.getProtoObj().mutable_enginesettings() = curEngineSettings_.getProtoBufEngineSettings();
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);

}

void EngineServer::onEnginePacketSizeDetectionStateChanged(bool on, bool isError)
{
    IPC::ProtobufCommand<IPCServerCommands::PacketSizeDetectionState> cmd;
    cmd.getProtoObj().set_on(on);
    cmd.getProtoObj().set_is_error(isError);
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::onHostsFileBecameWritable()
{
    IPC::ProtobufCommand<IPCServerCommands::HostsFileBecameWritable> cmd;
    sendCmdToAllAuthorizedAndGetStateClients(cmd, true);
}

void EngineServer::sendCmdToAllAuthorizedAndGetStateClients(const IPC::Command &cmd, bool bWithLog)
{
    // Only log this command one time.
    bool bLogged = false;

    sendCmdToAllAuthorizedAndGetStateClientsOfType(cmd, bWithLog, ProtoTypes::CLIENT_ID_GUI, &bLogged);
    sendCmdToAllAuthorizedAndGetStateClientsOfType(cmd, bWithLog, ProtoTypes::CLIENT_ID_CLI, &bLogged);

    if (bWithLog && !bLogged) {
        qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    }
}

void EngineServer::sendCmdToAllAuthorizedAndGetStateClientsOfType(const IPC::Command &cmd, bool bWithLog, unsigned int clientId, bool *bLogged)
{
    for (auto it = connections_.begin(); it != connections_.end(); ++it)
    {
        if (it.key() && it.value().bClientAuthReceived_ && it.value().clientId_ == clientId)
        {
            if (bWithLog)
            {
                qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
                if (bLogged != nullptr) {
                    *bLogged = true;
                }
            }

            it.key()->sendCommand(cmd);
        }
    }
}

