#include "backend.h"

#include "utils/logger.h"
#include "utils/utils.h"
#include "ipc/connection.h"
#include "ipc/protobufcommand.h"
#include "ipc/servercommands.h"
#include "ipc/clientcommands.h"
#include "utils/utils.h"
#include "persistentstate.h"
#include "engineserver.h"
#include <QCoreApplication>


const int typeIdNetworkInterface = qRegisterMetaType<ProtoTypes::NetworkInterface>("ProtoTypes::NetworkInterface");

Backend::Backend(QObject *parent) : QObject(parent),
    isSavedApiSettingsExists_(false),
    bLastLoginWithAuthHash_(false),
    isCleanupFinished_(false),
    cmdId_(0),
    isFirewallEnabled_(false),
    isExternalConfigMode_(false)
{
    preferences_.loadGuiSettings();

    locationsModel_ = new LocationsModel(this);

    connect(&connectStateHelper_, SIGNAL(connectStateChanged(ProtoTypes::ConnectState)), SIGNAL(connectStateChanged(ProtoTypes::ConnectState)));
    connect(&emergencyConnectStateHelper_, SIGNAL(connectStateChanged(ProtoTypes::ConnectState)), SIGNAL(emergencyConnectStateChanged(ProtoTypes::ConnectState)));
    connect(&firewallStateHelper_, SIGNAL(firewallStateChanged(bool)), SIGNAL(firewallStateChanged(bool)));

    engineServer_ = new EngineServer(this);
    connect(engineServer_, SIGNAL(emitCommand(IPC::Command*)), SLOT(onConnectionNewCommand(IPC::Command*)));
    connect(engineServer_, &EngineServer::wireGuardAtKeyLimit, this, &Backend::wireGuardAtKeyLimit);
    connect(this, &Backend::wireGuardKeyLimitUserResponse, engineServer_, &EngineServer::wireGuardKeyLimitUserResponse);
}

Backend::~Backend()
{
    SAFE_DELETE(engineServer_);
    preferences_.saveGuiSettings();
}

void Backend::init()
{
    isCleanupFinished_ = false;
    qCDebug(LOG_BASIC) << "Backend::init()";

    IPC::ProtobufCommand<IPCClientCommands::ClientAuth> cmd;
    cmd.getProtoObj().set_protocol_version(0);
    cmd.getProtoObj().set_client_id(0);
    cmd.getProtoObj().set_pid(0);
    cmd.getProtoObj().set_name("gui");
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::basicInit()
{
    /*qCDebug(LOG_BASIC) << "Backend::basicInit()";

    Q_ASSERT(connection_ == NULL);
    Q_ASSERT(process_ == NULL);

    connection_ = new IPC::Connection();
    connect(dynamic_cast<QObject*>(connection_), SIGNAL(newCommand(IPC::Command *, IPC::IConnection *)), SLOT(onConnectionNewCommand(IPC::Command *, IPC::IConnection *)), Qt::QueuedConnection);
    connect(dynamic_cast<QObject*>(connection_), SIGNAL(stateChanged(int, IPC::IConnection *)), SLOT(onConnectionStateChanged(int, IPC::IConnection *)), Qt::QueuedConnection);

    ipcState_ = IPC_CONNECTING;
    connectingTimer_.start();
    connection_->connect();*/
}

void Backend::basicClose()
{
    /*qCDebug(LOG_BASIC) << "Backend::closeConnectionWithoutCleanup()";
    ipcState_ = IPC_DOING_CLEANUP;
    connection_->close();*/
}

void Backend::cleanup(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart)
{
    qCDebug(LOG_BASIC) << "Backend::cleanup()";

    //Q_ASSERT(isInitFinished());

    /*if (ipcState_ >= IPC_CONNECTED && ipcState_ < IPC_READY)
    {
        ipcState_ = IPC_FINISHED_STATE;
        connection_->close();
    }
    else if (ipcState_ == IPC_READY)*/
    {
        IPC::ProtobufCommand<IPCClientCommands::Cleanup> cmd;
        cmd.getProtoObj().set_is_exit_with_restart(isExitWithRestart);
        cmd.getProtoObj().set_is_firewall_checked(isFirewallChecked);
        cmd.getProtoObj().set_is_firewall_always_on(isFirewallAlwaysOn);
        cmd.getProtoObj().set_is_launch_on_start(isLaunchOnStart);
        qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        engineServer_->sendCommand(&cmd);
    }
}

void Backend::enableBFE_win()
{
    IPC::ProtobufCommand<IPCClientCommands::EnableBfe_win> cmd;
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::login(const QString &username, const QString &password, const QString &code2fa)
{
    bLastLoginWithAuthHash_ = false;
    IPC::ProtobufCommand<IPCClientCommands::Login> cmd;
    cmd.getProtoObj().set_username(username.toStdString());
    cmd.getProtoObj().set_password(password.toStdString());
    cmd.getProtoObj().set_code2fa(code2fa.toStdString());
    cmd.getProtoObj().set_auth_hash("");

    // hide password for logging
    IPC::ProtobufCommand<IPCClientCommands::Login> loggedCmd = cmd;
    loggedCmd.getProtoObj().set_password("*****");
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(loggedCmd.getDebugString());

    engineServer_->sendCommand(&cmd);
}

bool Backend::isCanLoginWithAuthHash() const
{
    return !accountInfo_.authHash().isEmpty();
}

bool Backend::isSavedApiSettingsExists() const
{
    return isSavedApiSettingsExists_;
}

void Backend::loginWithAuthHash(const QString &authHash)
{
    bLastLoginWithAuthHash_ = true;
    IPC::ProtobufCommand<IPCClientCommands::Login> cmd;
    cmd.getProtoObj().set_auth_hash(authHash.toStdString());
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

QString Backend::getCurrentAuthHash() const
{
    return accountInfo_.authHash();
}

void Backend::loginWithLastLoginSettings()
{
    IPC::ProtobufCommand<IPCClientCommands::Login> cmd;
    cmd.getProtoObj().set_use_last_login_settings(true);
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

bool Backend::isLastLoginWithAuthHash() const
{
    return bLastLoginWithAuthHash_;
}

void Backend::signOut(bool keepFirewallOn)
{
    IPC::ProtobufCommand<IPCClientCommands::SignOut> cmd;
    cmd.getProtoObj().set_is_keep_firewall_on(keepFirewallOn);
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendConnect(const LocationID &lid)
{
    connectStateHelper_.connectClickFromUser();
    IPC::ProtobufCommand<IPCClientCommands::Connect> cmd;
    *cmd.getProtoObj().mutable_locationdid() = lid.toProtobuf();
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendDisconnect()
{
    connectStateHelper_.disconnectClickFromUser();
    IPC::ProtobufCommand<IPCClientCommands::Disconnect> cmd;
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

bool Backend::isDisconnected() const
{
    return connectStateHelper_.isDisconnected();
}

ProtoTypes::ConnectStateType Backend::currentConnectState() const
{
    return connectStateHelper_.currentConnectState().connect_state_type();
}

void Backend::forceCliStateUpdate()
{
    IPC::ProtobufCommand<IPCClientCommands::ForceCliStateUpdate> cmd;
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::firewallOn(bool updateHelperFirst)
{
    if (updateHelperFirst) firewallStateHelper_.firewallOnClickFromGUI();
    IPC::ProtobufCommand<IPCClientCommands::Firewall> cmd;
    cmd.getProtoObj().set_is_enable(true);
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::firewallOff(bool updateHelperFirst)
{
    if (updateHelperFirst) firewallStateHelper_.firewallOffClickFromGUI();
    IPC::ProtobufCommand<IPCClientCommands::Firewall> cmd;
    cmd.getProtoObj().set_is_enable(false);
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

bool Backend::isFirewallEnabled()
{
    return firewallStateHelper_.isEnabled();
}

bool Backend::isFirewallAlwaysOn()
{
    return getPreferences()->firewalSettings().mode == FIREWALL_MODE_ALWAYS_ON;
}

void Backend::emergencyConnectClick()
{
    emergencyConnectStateHelper_.connectClickFromUser();
    IPC::ProtobufCommand<IPCClientCommands::EmergencyConnect> cmd;
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::emergencyDisconnectClick()
{
    emergencyConnectStateHelper_.disconnectClickFromUser();
    IPC::ProtobufCommand<IPCClientCommands::EmergencyDisconnect> cmd;
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

bool Backend::isEmergencyDisconnected()
{
    return emergencyConnectStateHelper_.isDisconnected();
}

void Backend::startWifiSharing(const QString &ssid, const QString &password)
{
    IPC::ProtobufCommand<IPCClientCommands::StartWifiSharing> cmd;
    cmd.getProtoObj().set_ssid(ssid.toStdString());
    cmd.getProtoObj().set_password(password.toStdString());
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::stopWifiSharing()
{
    IPC::ProtobufCommand<IPCClientCommands::StopWifiSharing> cmd;
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::startProxySharing(ProtoTypes::ProxySharingMode proxySharingMode)
{
    IPC::ProtobufCommand<IPCClientCommands::StartProxySharing> cmd;
    cmd.getProtoObj().set_sharing_mode(proxySharingMode);
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::stopProxySharing()
{
   IPC::ProtobufCommand<IPCClientCommands::StopProxySharing> cmd;
   qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
   engineServer_->sendCommand(&cmd);
}

void Backend::setIPv6StateInOS(bool bEnabled)
{
    IPC::ProtobufCommand<IPCClientCommands::SetIpv6StateInOS> cmd;
    cmd.getProtoObj().set_is_enabled(bEnabled);
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::getAndUpdateIPv6StateInOS()
{
    IPC::ProtobufCommand<IPCClientCommands::GetIpv6StateInOS> cmd;
    // qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString()); // hidden because it's a bit spammy
    engineServer_->sendCommand(&cmd);
}

void Backend::gotoCustomOvpnConfigMode()
{
    IPC::ProtobufCommand<IPCClientCommands::GotoCustomOvpnConfigMode> cmd;
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::recordInstall()
{
    IPC::ProtobufCommand<IPCClientCommands::RecordInstall> cmd;
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendConfirmEmail()
{
    IPC::ProtobufCommand<IPCClientCommands::SendConfirmEmail> cmd;
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendDebugLog()
{
    IPC::ProtobufCommand<IPCClientCommands::SendDebugLog> cmd;
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::getWebSessionTokenForEditAccountDetails()
{
    IPC::ProtobufCommand<IPCClientCommands::GetWebSessionToken> cmd;
    cmd.getProtoObj().set_purpose(ProtoTypes::WEB_SESSION_PURPOSE_EDIT_ACCOUNT_DETAILS);
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::getWebSessionTokenForAddEmail()
{
    IPC::ProtobufCommand<IPCClientCommands::GetWebSessionToken> cmd;
    cmd.getProtoObj().set_purpose(ProtoTypes::WEB_SESSION_PURPOSE_ADD_EMAIL);
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::speedRating(int rating, const QString &localExternalIp)
{
    IPC::ProtobufCommand<IPCClientCommands::SpeedRating> cmd;
    cmd.getProtoObj().set_rating(rating);
    cmd.getProtoObj().set_local_external_ip(localExternalIp.toStdString());
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::setBlockConnect(bool isBlockConnect)
{
    IPC::ProtobufCommand<IPCClientCommands::SetBlockConnect> cmd;
    cmd.getProtoObj().set_is_block_connect(isBlockConnect);
    // qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString()); // a bit spammy
    engineServer_->sendCommand(&cmd);
}

void Backend::clearCredentials()
{
    IPC::ProtobufCommand<IPCClientCommands::ClearCredentials> cmd;
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

bool Backend::isAppCanClose() const
{
    return isCleanupFinished_;
}

void Backend::continueWithCredentialsForOvpnConfig(const QString &username, const QString &password, bool bSave)
{
    qCDebug(LOG_BASIC) << "Backend::continueWithCredentialsForOvpnConfig()";
    IPC::ProtobufCommand<IPCClientCommands::ContinueWithCredentialsForOvpnConfig> cmd;
    cmd.getProtoObj().set_username(username.toStdString());
    cmd.getProtoObj().set_password(password.toStdString());
    cmd.getProtoObj().set_is_save(bSave);
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendAdvancedParametersChanged()
{
    IPC::ProtobufCommand<IPCClientCommands::AdvancedParametersChanged> cmd;
    qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendEngineSettingsIfChanged()
{
    if (preferences_.getEngineSettings() != latestEngineSettings_)
    {
        qCDebug(LOG_BASIC) << "Engine settings changed, sent to engine";
        latestEngineSettings_ = preferences_.getEngineSettings();
        IPC::ClientCommands::SetSettings cmd(latestEngineSettings_);
        qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        engineServer_->sendCommand(&cmd);
    }
}

LocationsModel *Backend::getLocationsModel()
{
    return locationsModel_;
}

PreferencesHelper *Backend::getPreferencesHelper()
{
    return &preferencesHelper_;
}

Preferences *Backend::getPreferences()
{
    return &preferences_;
}

AccountInfo *Backend::getAccountInfo()
{
    return &accountInfo_;
}

void Backend::applicationActivated()
{
    IPC::ProtobufCommand<IPCClientCommands::ApplicationActivated> cmd;
    cmd.getProtoObj().set_is_activated(true);
    // qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::applicationDeactivated()
{
    IPC::ProtobufCommand<IPCClientCommands::ApplicationActivated> cmd;
    cmd.getProtoObj().set_is_activated(false);
    // qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

const types::SessionStatus &Backend::getSessionStatus() const
{
    return latestSessionStatus_;
}

void Backend::onConnectionNewCommand(IPC::Command *command)
{
    if (command->getStringId() == IPC::ServerCommands::AuthReply::getCommandStringId())
    {
        IPC::ProtobufCommand<IPCClientCommands::Init> cmd;
        qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        engineServer_->sendCommand(&cmd);
    }
    else if (command->getStringId() == IPC::ServerCommands::InitFinished::getCommandStringId())
    {
        // if (ipcState_ != IPC_READY) // safe? -- prevent triggering GUI initFinished when Engine Init is broadcast as a result of CLI init
        {
            IPC::ServerCommands::InitFinished *cmd = static_cast<IPC::ServerCommands::InitFinished *>(command);
            if (cmd->initState_ == INIT_STATE_SUCCESS)
            {
                latestEngineSettings_ = cmd->engineSettings_;
                preferences_.setEngineSettings(latestEngineSettings_);
                getOpenVpnVersionsFromInitCommand(*cmd);

                // WiFi sharing supported state
                if (cmd->isWifiSharingSupported_)
                {
                    preferencesHelper_.setWifiSharingSupported(cmd->isWifiSharingSupported_);
                }

                isSavedApiSettingsExists_ = cmd->isSavedApiSettingsExists_;
                accountInfo_.setAuthHash(cmd->authHash_);
            }
            Q_EMIT initFinished(cmd->initState_);
        }
    }
    else if (command->getStringId() == IPCServerCommands::FirewallStateChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::FirewallStateChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::FirewallStateChanged> *>(command);
        qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd->getDebugString());
        firewallStateHelper_.setFirewallStateFromEngine(cmd->getProtoObj().is_firewall_enabled());
    }
    else if (command->getStringId() == IPC::ServerCommands::LoginFinished::getCommandStringId())
    {
        IPC::ServerCommands::LoginFinished *cmd = static_cast<IPC::ServerCommands::LoginFinished *>(command);

        if (!cmd->authHash_.isEmpty())
        {
            accountInfo_.setAuthHash(cmd->authHash_);
        }
        preferencesHelper_.setPortMap(cmd->portMap_);

        Q_EMIT loginFinished(cmd->isLoginFromSettings_);
    }
    else if (command->getStringId() == IPCServerCommands::LoginStepMessage::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::LoginStepMessage> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::LoginStepMessage> *>(command);
        Q_EMIT loginStepMessage(cmd->getProtoObj().message());
    }
    else if (command->getStringId() == IPCServerCommands::LoginError::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::LoginError> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::LoginError> *>(command);
        Q_EMIT loginError(cmd->getProtoObj().error(), QString::fromStdString(cmd->getProtoObj().error_message()));
    }
    else if (command->getStringId() == IPC::ServerCommands::SessionStatusUpdated::getCommandStringId())
    {
        IPC::ServerCommands::SessionStatusUpdated *cmd = static_cast<IPC::ServerCommands::SessionStatusUpdated *>(command);
        latestSessionStatus_ = cmd->getSessionStatus();
        locationsModel_->setFreeSessionStatus(!latestSessionStatus_.isPremium());
        updateAccountInfo();
        Q_EMIT sessionStatusChanged(latestSessionStatus_);
    }
    else if (command->getStringId() == IPCServerCommands::LocationsUpdated::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::LocationsUpdated> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::LocationsUpdated> *>(command);
        locationsModel_->updateApiLocations(cmd->getProtoObj().best_location(), QString::fromStdString(cmd->getProtoObj().static_ip_device_name()), cmd->getProtoObj().locations());
        Q_EMIT locationsUpdated();
    }
    else if (command->getStringId() == IPCServerCommands::BestLocationUpdated::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::BestLocationUpdated> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::BestLocationUpdated> *>(command);
        locationsModel_->updateBestLocation(cmd->getProtoObj().best_location());
    }
    else if (command->getStringId() == IPCServerCommands::CustomConfigLocationsUpdated::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::CustomConfigLocationsUpdated> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::CustomConfigLocationsUpdated> *>(command);
        locationsModel_->updateCustomConfigLocations(cmd->getProtoObj().locations());
        Q_EMIT locationsUpdated();
    }
    else if (command->getStringId() == IPCServerCommands::LocationSpeedChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::LocationSpeedChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::LocationSpeedChanged> *>(command);
        locationsModel_->changeConnectionSpeed(LocationID::createFromProtoBuf(cmd->getProtoObj().id()), (int)cmd->getProtoObj().pingtime());
    }
    else if (command->getStringId() == IPCServerCommands::ConnectStateChanged::descriptor()->full_name())
    {
        qCDebugMultiline(LOG_IPC) << QString::fromStdString(command->getDebugString());
        IPC::ProtobufCommand<IPCServerCommands::ConnectStateChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::ConnectStateChanged> *>(command);
        connectStateHelper_.setConnectStateFromEngine(cmd->getProtoObj().connect_state());
    }
    else if (command->getStringId() == IPCServerCommands::EmergencyConnectStateChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::EmergencyConnectStateChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::EmergencyConnectStateChanged> *>(command);
        emergencyConnectStateHelper_.setConnectStateFromEngine(cmd->getProtoObj().emergency_connect_state());
    }
    else if (command->getStringId() == IPCServerCommands::ProxySharingInfoChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::ProxySharingInfoChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::ProxySharingInfoChanged> *>(command);
        Q_EMIT proxySharingInfoChanged(cmd->getProtoObj().proxy_sharing_info());
    }
    else if (command->getStringId() == IPCServerCommands::WifiSharingInfoChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::WifiSharingInfoChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::WifiSharingInfoChanged> *>(command);
        Q_EMIT wifiSharingInfoChanged(cmd->getProtoObj().wifi_sharing_info());
    }
    else if (command->getStringId() == IPCServerCommands::SignOutFinished::descriptor()->full_name())
    {
        // The engine has completed user sign out.  Clear any auth hash we may have stored.
        accountInfo_.setAuthHash(QString());
        Q_EMIT signOutFinished();
    }
    else if (command->getStringId() == IPCServerCommands::NotificationsUpdated::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::NotificationsUpdated> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::NotificationsUpdated> *>(command);
        Q_EMIT notificationsChanged(cmd->getProtoObj().array_notifications());
    }
    else if (command->getStringId() == IPCServerCommands::CheckUpdateInfoUpdated::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::CheckUpdateInfoUpdated> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::CheckUpdateInfoUpdated> *>(command);
        Q_EMIT checkUpdateChanged(cmd->getProtoObj().check_update_info());
    }
    else if (command->getStringId() == IPCServerCommands::MyIpUpdated::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::MyIpUpdated> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::MyIpUpdated> *>(command);
        Q_EMIT myIpChanged(QString::fromStdString(cmd->getProtoObj().my_ip_info().ip()), cmd->getProtoObj().my_ip_info().is_disconnected_state());
    }
    else if (command->getStringId() == IPCServerCommands::StatisticsUpdated::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::StatisticsUpdated> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::StatisticsUpdated> *>(command);
        Q_EMIT statisticsUpdated(cmd->getProtoObj().bytes_in(), cmd->getProtoObj().bytes_out(), cmd->getProtoObj().is_total_bytes());
    }
    else if (command->getStringId() == IPCServerCommands::RequestCredentialsForOvpnConfig::descriptor()->full_name())
    {
        Q_EMIT requestCustomOvpnConfigCredentials();
    }
    else if (command->getStringId() == IPCServerCommands::DebugLogResult::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::DebugLogResult> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::DebugLogResult> *>(command);
        Q_EMIT debugLogResult(cmd->getProtoObj().success());
    }
    else if (command->getStringId() == IPCServerCommands::ConfirmEmailResult::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::ConfirmEmailResult> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::ConfirmEmailResult> *>(command);
        Q_EMIT confirmEmailResult(cmd->getProtoObj().success());
    }
    else if (command->getStringId() == IPCServerCommands::Ipv6StateInOS::descriptor()->full_name())
    {
#ifdef Q_OS_WIN
        IPC::ProtobufCommand<IPCServerCommands::Ipv6StateInOS> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::Ipv6StateInOS> *>(command);
        preferencesHelper_.setIpv6StateInOS(cmd->getProtoObj().is_enabled());
#endif
    }
    else if (command->getStringId() == IPCServerCommands::CustomOvpnConfigModeInitFinished::descriptor()->full_name())
    {
        Q_EMIT gotoCustomOvpnConfigModeFinished();
    }
    else if (command->getStringId() == IPC::ServerCommands::EngineSettingsChanged::getCommandStringId())
    {
        qCDebugMultiline(LOG_IPC) << QString::fromStdString(command->getDebugString());
        latestEngineSettings_ = static_cast<IPC::ServerCommands::EngineSettingsChanged *>(command)->engineSettings_;
        preferences_.setEngineSettings(latestEngineSettings_);
    }
    else if (command->getStringId() == IPCServerCommands::CleanupFinished::descriptor()->full_name())
    {
        isCleanupFinished_ = true;
        Q_EMIT cleanupFinished();
    }
    else if (command->getStringId() == IPC::ServerCommands::NetworkChanged::getCommandStringId())
    {
        qCDebugMultiline(LOG_IPC) << QString::fromStdString(command->getDebugString());
        IPC::ServerCommands::NetworkChanged *cmd = static_cast<IPC::ServerCommands::NetworkChanged *>(command);
        handleNetworkChange(cmd->networkInterface_);
    }
    else if (command->getStringId() == IPCServerCommands::SessionDeleted::descriptor()->full_name())
    {
        Q_EMIT sessionDeleted();
    }
    else if (command->getStringId() == IPCServerCommands::TestTunnelResult::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::TestTunnelResult> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::TestTunnelResult> *>(command);
        Q_EMIT testTunnelResult(cmd->getProtoObj().success());
    }
    else if (command->getStringId() == IPCServerCommands::LostConnectionToHelper::descriptor()->full_name())
    {
        Q_EMIT lostConnectionToHelper();
    }
    else if (command->getStringId() == IPCServerCommands::HighCpuUsage::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::HighCpuUsage> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::HighCpuUsage> *>(command);
        QStringList list;
        for (int i = 0; i < cmd->getProtoObj().processes_size(); ++i)
        {
            list << QString::fromStdString(cmd->getProtoObj().processes(i));
        }
        Q_EMIT highCpuUsage(list);
    }
    else if (command->getStringId() == IPCServerCommands::UserWarning::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::UserWarning> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::UserWarning> *>(command);
        Q_EMIT userWarning(cmd->getProtoObj().type());
    }
    else if (command->getStringId() == IPCServerCommands::InternetConnectivityChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::InternetConnectivityChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::InternetConnectivityChanged> *>(command);
        Q_EMIT internetConnectivityChanged(cmd->getProtoObj().connectivity());
    }
    else if (command->getStringId() == IPCServerCommands::ProtocolPortChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::ProtocolPortChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::ProtocolPortChanged> *>(command);
        Q_EMIT protocolPortChanged(cmd->getProtoObj().protocol(), cmd->getProtoObj().port());
    }
    else if (command->getStringId() == IPCServerCommands::PacketSizeDetectionState::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::PacketSizeDetectionState> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::PacketSizeDetectionState> *>(command);
        Q_EMIT packetSizeDetectionStateChanged(cmd->getProtoObj().on(), cmd->getProtoObj().is_error());
    }
    else if (command->getStringId() == IPCServerCommands::UpdateVersionChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::UpdateVersionChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::UpdateVersionChanged> *>(command);
        Q_EMIT updateVersionChanged(cmd->getProtoObj().progress(), cmd->getProtoObj().state(), cmd->getProtoObj().error());
    }
    else if (command->getStringId() == IPCServerCommands::HostsFileBecameWritable::descriptor()->full_name())
    {
        qCDebug(LOG_BASIC) << "Hosts file became writable -- Connecting..";
        sendConnect(PersistentState::instance().lastLocation());
    }
    else if (command->getStringId() == IPCServerCommands::WebSessionToken::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::WebSessionToken> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::WebSessionToken> *>(command);
        if (cmd->getProtoObj().purpose() == ProtoTypes::WEB_SESSION_PURPOSE_EDIT_ACCOUNT_DETAILS)
        {
            Q_EMIT webSessionTokenForEditAccountDetails(QString::fromStdString(cmd->getProtoObj().temp_session_token()));
        }
        else if (cmd->getProtoObj().purpose() == ProtoTypes::WEB_SESSION_PURPOSE_ADD_EMAIL)
        {
            Q_EMIT webSessionTokenForAddEmail(QString::fromStdString(cmd->getProtoObj().temp_session_token()));
        }
    }
}

void Backend::abortInitialization()
{
    /*if (ipcState_ != IPC_CONNECTING)
        return;

    connectionAttemptTimer_.stop();

    qCDebug(LOG_BASIC) << "Connection to engine server aborted by user";
    if (connection_)
        connection_->close();*/

    Q_EMIT initFinished(INIT_STATE_CLEAN);
}

// Assumes that duplicate network filtering occurs on Engine side
void Backend::handleNetworkChange(types::NetworkInterface networkInterface, bool manual)
{
    bool newNetwork = true;

    // find or assign friendly name before checking is network is the same as current network
    QString friendlyName = networkInterface.networkOrSSid;

    QVector<types::NetworkInterface> networkListOld = PersistentState::instance().networkWhitelist();
    for (int i = 0; i < networkListOld.size(); i++)
    {
        if (networkListOld[i].networkOrSSid== networkInterface.networkOrSSid)
        {
            friendlyName = networkListOld[i].friendlyName;
            newNetwork = false;
            break;
        }
    }

    if (friendlyName == "") friendlyName = networkInterface.networkOrSSid;
    networkInterface.friendlyName = friendlyName;

    if (networkInterface.networkOrSSid != "") // not a disconnect
    {
        // Add a new network as secured
        if (newNetwork)
        {

#ifdef Q_OS_MAC
            // generate friendly name for MacOS Ethernet
            if (networkInterface.interface_type() == ProtoTypes::NETWORK_INTERFACE_ETH)
            {
                friendlyName = generateNewFriendlyName();
                networkInterface.set_friendly_name(friendlyName.toStdString().c_str());

            }
#endif
            types::NetworkInterface newEntry;
            newEntry = networkInterface;
            networkListOld << newEntry;
            preferences_.setNetworkWhiteList(networkListOld);
        }

        // GUI-side persistent list holds trustiness
        QVector<types::NetworkInterface> networkList = PersistentState::instance().networkWhitelist();
        types::NetworkInterface foundInterface;
        for (int i = 0; i < networkList.size(); i++)
        {
            if (networkList[i].networkOrSSid == networkInterface.networkOrSSid)
            {
                foundInterface = networkList[i];
                break;
            }
        }

        if (!Utils::sameNetworkInterface(networkInterface, currentNetworkInterface_) || manual)
            // actual network change or explicit trigger from preference change
            // prevents brief/rare network loss during CONNECTING from triggering network change
        {
            // disconnect VPN on an unsecured network -- connect VPN on a secured network if auto-connect is on
            if (foundInterface.trustType == NETWORK_TRUST_UNSECURED)
            {
                if (!connectStateHelper_.isDisconnected())
                {
                    qCDebug(LOG_BASIC) << "Network Whitelisting detected UNSECURED network -- Disconnecting..";
                    sendDisconnect();
                }
            }
            else // SECURED
            {
                if (connectStateHelper_.isDisconnected())
                {
                    if (preferences_.isAutoConnect())
                    {
                        qCDebug(LOG_BASIC) << "Network Whitelisting detected SECURED network -- Connecting..";
                        sendConnect(PersistentState::instance().lastLocation());
                    }
                }
            }

            currentNetworkInterface_ = networkInterface;
        }

		// Even if not a real network change we want to update the UI with current network info.
        types::NetworkInterface protoInterface = networkInterface;
        protoInterface.trustType =foundInterface.trustType;
        Q_EMIT networkChanged(protoInterface);

    }
    else
    {
        // inform UI no network
        Q_EMIT networkChanged(networkInterface);
    }
}

types::NetworkInterface Backend::getCurrentNetworkInterface()
{
    return currentNetworkInterface_;
}

void Backend::cycleMacAddress()
{
    QString macAddress = Utils::generateRandomMacAddress(); // TODO: move generation into Engine (BEWARE: mac change only occurs on preferences closing and when manual MAC cycled)

    types::MacAddrSpoofing mas = preferences_.macAddrSpoofing();
    mas.macAddress = macAddress;
    preferences_.setMacAddrSpoofing(mas);

    sendEngineSettingsIfChanged(); //  force EngineSettings update on manual MAC cycle button press
}

void Backend::sendDetectPacketSize()
{
    IPC::ProtobufCommand<IPCClientCommands::DetectPacketSize> cmd;
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendSplitTunneling(ProtoTypes::SplitTunneling st)
{
    IPC::ProtobufCommand<IPCClientCommands::SplitTunneling> cmd;
    *cmd.getProtoObj().mutable_split_tunneling() = st;
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
    Q_EMIT splitTunnelingStateChanged(st.settings().active());
}

void Backend::sendUpdateWindowInfo(qint32 mainWindowCenterX, qint32 mainWindowCenterY)
{
    IPC::ProtobufCommand<IPCClientCommands::UpdateWindowInfo> cmd;
    cmd.getProtoObj().set_window_center_x(mainWindowCenterX);
    cmd.getProtoObj().set_window_center_y(mainWindowCenterY);
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendUpdateVersion(qint64 mainWindowHandle)
{
    IPC::ProtobufCommand<IPCClientCommands::UpdateVersion> cmd;
    cmd.getProtoObj().set_hwnd(mainWindowHandle);
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::cancelUpdateVersion()
{
    IPC::ProtobufCommand<IPCClientCommands::UpdateVersion> cmd;
    cmd.getProtoObj().set_cancel_download(true);
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendMakeHostsFilesWritableWin()
{
    IPC::ProtobufCommand<IPCClientCommands::MakeHostsWritableWin> cmd;
    qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

QString Backend::generateNewFriendlyName()
{
    QList<QString> friendlyNames;
    QVector<types::NetworkInterface> whiteList = preferences_.networkWhiteList();
    for (int i = 0; i < whiteList.size(); i++)
    {
        types::NetworkInterface network = whiteList[i];
        friendlyNames.append(network.friendlyName);
    }

    int newIndex = 0;
    QString newFriendlyName = "Network " + QString::number(newIndex);
    while (friendlyNames.contains(newFriendlyName))
    {
        newIndex++;
        newFriendlyName = "Network " + QString::number(newIndex);
    }

    return newFriendlyName;
}

void Backend::updateAccountInfo()
{
    accountInfo_.setEmail(latestSessionStatus_.getEmail());
    accountInfo_.setNeedConfirmEmail(latestSessionStatus_.getEmailStatus() == 0);
    accountInfo_.setUsername(latestSessionStatus_.getUsername());
    accountInfo_.setExpireDate(latestSessionStatus_.getPremiumExpireDate());
    accountInfo_.setPlan(latestSessionStatus_.getTrafficMax());
    accountInfo_.setIsPremium(latestSessionStatus_.isPremium());
}

void Backend::getOpenVpnVersionsFromInitCommand(const IPC::ServerCommands::InitFinished &cmd)
{
    // OpenVpn versions
    if (cmd.availableOpenvpnVersions_.size() > 0)
    {
        QStringList list;
        for (auto it : cmd.availableOpenvpnVersions_)
        {
            list << it;
        }

        preferencesHelper_.setAvailableOpenVpnVersions(list);
    }
}
