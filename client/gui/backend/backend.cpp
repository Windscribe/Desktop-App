#include "backend.h"

#include "utils/logger.h"
#include "utils/utils.h"
#include "ipc/servercommands.h"
#include "ipc/clientcommands.h"
#include "utils/utils.h"
#include "persistentstate.h"
#include "engineserver.h"
#include <QCoreApplication>


Backend::Backend(QObject *parent) : QObject(parent),
    isSavedApiSettingsExists_(false),
    bLastLoginWithAuthHash_(false),
    isCleanupFinished_(false),
    cmdId_(0),
    isFirewallEnabled_(false),
    isExternalConfigMode_(false)
{
    preferences_.loadGuiSettings();

    locationsModelManager_ = new gui_locations::LocationsModelManager(this);

    connect(&connectStateHelper_, SIGNAL(connectStateChanged(types::ConnectState)), SIGNAL(connectStateChanged(types::ConnectState)));
    connect(&emergencyConnectStateHelper_, SIGNAL(connectStateChanged(types::ConnectState)), SIGNAL(emergencyConnectStateChanged(types::ConnectState)));
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

    IPC::ClientCommands::ClientAuth cmd;
    engineServer_->sendCommand(&cmd);
}

void Backend::cleanup(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart)
{
    qCDebug(LOG_BASIC) << "Backend::cleanup()";

    IPC::ClientCommands::Cleanup cmd;
    cmd.isExitWithRestart_ = isExitWithRestart;
    cmd.isFirewallChecked_ = isFirewallChecked;
    cmd.isFirewallAlwaysOn_ = isFirewallAlwaysOn;
    cmd.isLaunchOnStartup_ = isLaunchOnStart;

    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::enableBFE_win()
{
    IPC::ClientCommands::EnableBfe_win cmd;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::login(const QString &username, const QString &password, const QString &code2fa)
{
    bLastLoginWithAuthHash_ = false;
    IPC::ClientCommands::Login cmd;
    cmd.username_ = username;
    cmd.password_ = password;
    cmd.code2fa_ = code2fa;
    cmd.authHash_.clear();

    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
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
    IPC::ClientCommands::Login cmd;
    cmd.authHash_ = authHash;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

QString Backend::getCurrentAuthHash() const
{
    return accountInfo_.authHash();
}

void Backend::loginWithLastLoginSettings()
{
    IPC::ClientCommands::Login cmd;
    cmd.useLastLoginSettings = true;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

bool Backend::isLastLoginWithAuthHash() const
{
    return bLastLoginWithAuthHash_;
}

void Backend::signOut(bool keepFirewallOn)
{
    IPC::ClientCommands::SignOut cmd;
    cmd.isKeepFirewallOn_ = keepFirewallOn;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendConnect(const LocationID &lid)
{
    connectStateHelper_.connectClickFromUser();
    IPC::ClientCommands::Connect cmd;
    cmd.locationId_ = lid;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendDisconnect()
{
    connectStateHelper_.disconnectClickFromUser();
    IPC::ClientCommands::Disconnect cmd;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

bool Backend::isDisconnected() const
{
    return connectStateHelper_.isDisconnected();
}

CONNECT_STATE Backend::currentConnectState() const
{
    return connectStateHelper_.currentConnectState().connectState;
}

void Backend::firewallOn(bool updateHelperFirst)
{
    if (updateHelperFirst) firewallStateHelper_.firewallOnClickFromGUI();
    IPC::ClientCommands::Firewall cmd;
    cmd.isEnable_ = true;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::firewallOff(bool updateHelperFirst)
{
    if (updateHelperFirst) firewallStateHelper_.firewallOffClickFromGUI();
    IPC::ClientCommands::Firewall cmd;
    cmd.isEnable_ = false;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

bool Backend::isFirewallEnabled()
{
    return firewallStateHelper_.isEnabled();
}

bool Backend::isFirewallAlwaysOn()
{
    return getPreferences()->firewallSettings().mode == FIREWALL_MODE_ALWAYS_ON;
}

void Backend::emergencyConnectClick()
{
    emergencyConnectStateHelper_.connectClickFromUser();
    IPC::ClientCommands::EmergencyConnect cmd;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::emergencyDisconnectClick()
{
    emergencyConnectStateHelper_.disconnectClickFromUser();
    IPC::ClientCommands::EmergencyDisconnect cmd;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

bool Backend::isEmergencyDisconnected()
{
    return emergencyConnectStateHelper_.isDisconnected();
}

void Backend::startWifiSharing(const QString &ssid, const QString &password)
{
    IPC::ClientCommands::StartWifiSharing cmd;
    cmd.ssid_ = ssid;
    cmd.password_ = password;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::stopWifiSharing()
{
    IPC::ClientCommands::StopWifiSharing cmd;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::startProxySharing(PROXY_SHARING_TYPE proxySharingMode)
{
    IPC::ClientCommands::StartProxySharing cmd;
    cmd.sharingMode_ = proxySharingMode;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::stopProxySharing()
{
   IPC::ClientCommands::StopProxySharing cmd;
   //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
   engineServer_->sendCommand(&cmd);
}

void Backend::setIPv6StateInOS(bool bEnabled)
{
    IPC::ClientCommands::SetIpv6StateInOS cmd;
    cmd.isEnabled_ = bEnabled;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::getAndUpdateIPv6StateInOS()
{
    IPC::ClientCommands::GetIpv6StateInOS cmd;
    engineServer_->sendCommand(&cmd);
}

void Backend::gotoCustomOvpnConfigMode()
{
    IPC::ClientCommands::GotoCustomOvpnConfigMode cmd;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::recordInstall()
{
    IPC::ClientCommands::RecordInstall cmd;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendConfirmEmail()
{
    IPC::ClientCommands::SendConfirmEmail cmd;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendDebugLog()
{
    IPC::ClientCommands::SendDebugLog cmd;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::getWebSessionTokenForManageAccount()
{
    IPC::ClientCommands::GetWebSessionToken cmd;
    cmd.purpose_ = WEB_SESSION_PURPOSE_MANAGE_ACCOUNT;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::getWebSessionTokenForAddEmail()
{
    IPC::ClientCommands::GetWebSessionToken cmd;
    cmd.purpose_ = WEB_SESSION_PURPOSE_ADD_EMAIL;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::getWebSessionTokenForManageRobertRules()
{
    IPC::ClientCommands::GetWebSessionToken cmd;
    cmd.purpose_ = WEB_SESSION_PURPOSE_MANAGE_ROBERT_RULES;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::speedRating(int rating, const QString &localExternalIp)
{
    IPC::ClientCommands::SpeedRating cmd;
    cmd.rating_ = rating;
    cmd.localExternalIp_ = localExternalIp;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::setBlockConnect(bool isBlockConnect)
{
    IPC::ClientCommands::SetBlockConnect cmd;
    cmd.isBlockConnect_ = isBlockConnect;
    engineServer_->sendCommand(&cmd);
}

void Backend::clearCredentials()
{
    IPC::ClientCommands::ClearCredentials cmd;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::getRobertFilters()
{
    IPC::ClientCommands::GetRobertFilters cmd;
    engineServer_->sendCommand(&cmd);
}

void Backend::setRobertFilter(const types::RobertFilter &filter)
{
    IPC::ClientCommands::SetRobertFilter cmd;
    cmd.filter_ = filter;
    engineServer_->sendCommand(&cmd);
}

bool Backend::isAppCanClose() const
{
    return isCleanupFinished_;
}

void Backend::continueWithCredentialsForOvpnConfig(const QString &username, const QString &password, bool bSave)
{
    //qCDebug(LOG_BASIC) << "Backend::continueWithCredentialsForOvpnConfig()";
    IPC::ClientCommands::ContinueWithCredentialsForOvpnConfig cmd;
    cmd.username_ = username;
    cmd.password_ = password;
    cmd.isSave_ = bSave;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendAdvancedParametersChanged()
{
    IPC::ClientCommands::AdvancedParametersChanged cmd;
    //qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendEngineSettingsIfChanged()
{
    if (preferences_.getEngineSettings() != latestEngineSettings_)
    {
        //qCDebug(LOG_BASIC) << "Engine settings changed, sent to engine";
        latestEngineSettings_ = preferences_.getEngineSettings();
        IPC::ClientCommands::SetSettings cmd(latestEngineSettings_);
        //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        engineServer_->sendCommand(&cmd);
    }
}

gui_locations::LocationsModelManager *Backend::locationsModelManager()
{
    return locationsModelManager_;
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
    IPC::ClientCommands::ApplicationActivated cmd;
    cmd.isActivated_ = true;
    engineServer_->sendCommand(&cmd);
}

void Backend::applicationDeactivated()
{
    IPC::ClientCommands::ApplicationActivated cmd;
    cmd.isActivated_ = false;
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
        IPC::ClientCommands::Init cmd;
        //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
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
    else if (command->getStringId() == IPC::ServerCommands::FirewallStateChanged::getCommandStringId())
    {
        IPC::ServerCommands::FirewallStateChanged *cmd = static_cast<IPC::ServerCommands::FirewallStateChanged *>(command);
        //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd->getDebugString());
        firewallStateHelper_.setFirewallStateFromEngine(cmd->isFirewallEnabled_);
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
    else if (command->getStringId() == IPC::ServerCommands::LoginStepMessage::getCommandStringId())
    {
        IPC::ServerCommands::LoginStepMessage *cmd = static_cast<IPC::ServerCommands::LoginStepMessage *>(command);
        Q_EMIT loginStepMessage(cmd->message_);
    }
    else if (command->getStringId() == IPC::ServerCommands::LoginError::getCommandStringId())
    {
        IPC::ServerCommands::LoginError *cmd = static_cast<IPC::ServerCommands::LoginError *>(command);
        Q_EMIT loginError(cmd->error_, cmd->errorMessage_);
    }
    else if (command->getStringId() == IPC::ServerCommands::SessionStatusUpdated::getCommandStringId())
    {
        IPC::ServerCommands::SessionStatusUpdated *cmd = static_cast<IPC::ServerCommands::SessionStatusUpdated *>(command);
        latestSessionStatus_ = cmd->getSessionStatus();
        locationsModelManager_->setFreeSessionStatus(!latestSessionStatus_.isPremium());
        updateAccountInfo();
        Q_EMIT sessionStatusChanged(latestSessionStatus_);
    }
    else if (command->getStringId() == IPC::ServerCommands::LocationsUpdated::getCommandStringId())
    {
        IPC::ServerCommands::LocationsUpdated *cmd = static_cast<IPC::ServerCommands::LocationsUpdated *>(command);
        locationsModelManager_->updateLocations(cmd->bestLocation_, cmd->locations_);
        locationsModelManager_->updateDeviceName(cmd->staticIpDeviceName_);
    }
    else if (command->getStringId() == IPC::ServerCommands::BestLocationUpdated::getCommandStringId())
    {
        IPC::ServerCommands::BestLocationUpdated *cmd = static_cast<IPC::ServerCommands::BestLocationUpdated *>(command);
        locationsModelManager_->updateBestLocation(cmd->bestLocation_);
    }
    else if (command->getStringId() == IPC::ServerCommands::CustomConfigLocationsUpdated::getCommandStringId())
    {
        IPC::ServerCommands::CustomConfigLocationsUpdated *cmd = static_cast<IPC::ServerCommands::CustomConfigLocationsUpdated *>(command);
        locationsModelManager_->updateCustomConfigLocation(cmd->location_);
    }
    else if (command->getStringId() == IPC::ServerCommands::LocationSpeedChanged::getCommandStringId())
    {
        IPC::ServerCommands::LocationSpeedChanged *cmd = static_cast<IPC::ServerCommands::LocationSpeedChanged *>(command);
        locationsModelManager_->changeConnectionSpeed(cmd->id_, cmd->pingTime_);
    }
    else if (command->getStringId() == IPC::ServerCommands::ConnectStateChanged::getCommandStringId())
    {
        //qCDebugMultiline(LOG_IPC) << QString::fromStdString(command->getDebugString());
        IPC::ServerCommands::ConnectStateChanged *cmd = static_cast<IPC::ServerCommands::ConnectStateChanged *>(command);
        connectStateHelper_.setConnectStateFromEngine(cmd->connectState_);
    }
    else if (command->getStringId() == IPC::ServerCommands::EmergencyConnectStateChanged::getCommandStringId())
    {
        IPC::ServerCommands::EmergencyConnectStateChanged *cmd = static_cast<IPC::ServerCommands::EmergencyConnectStateChanged *>(command);
        emergencyConnectStateHelper_.setConnectStateFromEngine(cmd->emergencyConnectState_);
    }
    else if (command->getStringId() == IPC::ServerCommands::ProxySharingInfoChanged::getCommandStringId())
    {
        IPC::ServerCommands::ProxySharingInfoChanged *cmd = static_cast<IPC::ServerCommands::ProxySharingInfoChanged *>(command);
        Q_EMIT proxySharingInfoChanged(cmd->proxySharingInfo_);
    }
    else if (command->getStringId() == IPC::ServerCommands::WifiSharingInfoChanged::getCommandStringId())
    {
        IPC::ServerCommands::WifiSharingInfoChanged *cmd = static_cast<IPC::ServerCommands::WifiSharingInfoChanged *>(command);
        Q_EMIT wifiSharingInfoChanged(cmd->wifiSharingInfo_);
    }
    else if (command->getStringId() == IPC::ServerCommands::SignOutFinished::getCommandStringId())
    {
        // The engine has completed user sign out.  Clear any auth hash we may have stored.
        accountInfo_.setAuthHash(QString());
        Q_EMIT signOutFinished();
    }
    else if (command->getStringId() == IPC::ServerCommands::NotificationsUpdated::getCommandStringId())
    {
        IPC::ServerCommands::NotificationsUpdated *cmd = static_cast<IPC::ServerCommands::NotificationsUpdated *>(command);
        Q_EMIT notificationsChanged(cmd->notifications_);
    }
    else if (command->getStringId() == IPC::ServerCommands::CheckUpdateInfoUpdated::getCommandStringId())
    {
        IPC::ServerCommands::CheckUpdateInfoUpdated *cmd = static_cast<IPC::ServerCommands::CheckUpdateInfoUpdated *>(command);
        Q_EMIT checkUpdateChanged(cmd->checkUpdateInfo_);
    }
    else if (command->getStringId() == IPC::ServerCommands::MyIpUpdated::getCommandStringId())
    {
        IPC::ServerCommands::MyIpUpdated *cmd = static_cast<IPC::ServerCommands::MyIpUpdated *>(command);
        Q_EMIT myIpChanged(cmd->ip_, cmd->isDisconnectedState_);
    }
    else if (command->getStringId() == IPC::ServerCommands::StatisticsUpdated::getCommandStringId())
    {
        IPC::ServerCommands::StatisticsUpdated *cmd = static_cast<IPC::ServerCommands::StatisticsUpdated *>(command);
        Q_EMIT statisticsUpdated(cmd->bytesIn_, cmd->bytesOut_, cmd->isTotalBytes_);
    }
    else if (command->getStringId() == IPC::ServerCommands::RequestCredentialsForOvpnConfig::getCommandStringId())
    {
        Q_EMIT requestCustomOvpnConfigCredentials();
    }
    else if (command->getStringId() == IPC::ServerCommands::DebugLogResult::getCommandStringId())
    {
        IPC::ServerCommands::DebugLogResult *cmd = static_cast<IPC::ServerCommands::DebugLogResult *>(command);
        Q_EMIT debugLogResult(cmd->success_);
    }
    else if (command->getStringId() == IPC::ServerCommands::ConfirmEmailResult::getCommandStringId())
    {
        IPC::ServerCommands::ConfirmEmailResult *cmd = static_cast<IPC::ServerCommands::ConfirmEmailResult *>(command);
        Q_EMIT confirmEmailResult(cmd->success_);
    }
    else if (command->getStringId() == IPC::ServerCommands::Ipv6StateInOS::getCommandStringId())
    {
#ifdef Q_OS_WIN
        IPC::ServerCommands::Ipv6StateInOS *cmd = static_cast<IPC::ServerCommands::Ipv6StateInOS *>(command);
        preferencesHelper_.setIpv6StateInOS(cmd->isEnabled_);
#endif
    }
    else if (command->getStringId() == IPC::ServerCommands::CustomOvpnConfigModeInitFinished::getCommandStringId())
    {
        Q_EMIT gotoCustomOvpnConfigModeFinished();
    }
    else if (command->getStringId() == IPC::ServerCommands::EngineSettingsChanged::getCommandStringId())
    {
        //qCDebugMultiline(LOG_IPC) << QString::fromStdString(command->getDebugString());
        latestEngineSettings_ = static_cast<IPC::ServerCommands::EngineSettingsChanged *>(command)->engineSettings_;
        preferences_.setEngineSettings(latestEngineSettings_);
    }
    else if (command->getStringId() == IPC::ServerCommands::CleanupFinished::getCommandStringId())
    {
        isCleanupFinished_ = true;
        Q_EMIT cleanupFinished();
    }
    else if (command->getStringId() == IPC::ServerCommands::NetworkChanged::getCommandStringId())
    {
        //qCDebugMultiline(LOG_IPC) << QString::fromStdString(command->getDebugString());
        IPC::ServerCommands::NetworkChanged *cmd = static_cast<IPC::ServerCommands::NetworkChanged *>(command);
        handleNetworkChange(cmd->networkInterface_);
    }
    else if (command->getStringId() == IPC::ServerCommands::SessionDeleted::getCommandStringId())
    {
        Q_EMIT sessionDeleted();
    }
    else if (command->getStringId() == IPC::ServerCommands::TestTunnelResult::getCommandStringId())
    {
        IPC::ServerCommands::TestTunnelResult *cmd = static_cast<IPC::ServerCommands::TestTunnelResult *>(command);
        Q_EMIT testTunnelResult(cmd->success_);
    }
    else if (command->getStringId() == IPC::ServerCommands::LostConnectionToHelper::getCommandStringId())
    {
        Q_EMIT lostConnectionToHelper();
    }
    else if (command->getStringId() == IPC::ServerCommands::HighCpuUsage::getCommandStringId())
    {
        IPC::ServerCommands::HighCpuUsage *cmd = static_cast<IPC::ServerCommands::HighCpuUsage *>(command);
        Q_EMIT highCpuUsage(cmd->processes_);
    }
    else if (command->getStringId() == IPC::ServerCommands::UserWarning::getCommandStringId())
    {
        IPC::ServerCommands::UserWarning *cmd = static_cast<IPC::ServerCommands::UserWarning *>(command);
        Q_EMIT userWarning(cmd->type_);
    }
    else if (command->getStringId() == IPC::ServerCommands::InternetConnectivityChanged::getCommandStringId())
    {
        IPC::ServerCommands::InternetConnectivityChanged *cmd = static_cast<IPC::ServerCommands::InternetConnectivityChanged *>(command);
        Q_EMIT internetConnectivityChanged(cmd->connectivity_);
    }
    else if (command->getStringId() == IPC::ServerCommands::ProtocolPortChanged::getCommandStringId())
    {
        IPC::ServerCommands::ProtocolPortChanged *cmd = static_cast<IPC::ServerCommands::ProtocolPortChanged *>(command);
        Q_EMIT protocolPortChanged(cmd->protocol_, cmd->port_);
    }
    else if (command->getStringId() == IPC::ServerCommands::PacketSizeDetectionState::getCommandStringId())
    {
        IPC::ServerCommands::PacketSizeDetectionState *cmd = static_cast<IPC::ServerCommands::PacketSizeDetectionState *>(command);
        Q_EMIT packetSizeDetectionStateChanged(cmd->on_, cmd->isError_);
    }
    else if (command->getStringId() == IPC::ServerCommands::UpdateVersionChanged::getCommandStringId())
    {
        IPC::ServerCommands::UpdateVersionChanged *cmd = static_cast<IPC::ServerCommands::UpdateVersionChanged *>(command);
        Q_EMIT updateVersionChanged(cmd->progressPercent, cmd->state, cmd->error);
    }
    else if (command->getStringId() == IPC::ServerCommands::HostsFileBecameWritable::getCommandStringId())
    {
        qCDebug(LOG_BASIC) << "Hosts file became writable -- Connecting..";
        sendConnect(PersistentState::instance().lastLocation());
    }
    else if (command->getStringId() == IPC::ServerCommands::WebSessionToken::getCommandStringId())
    {
        IPC::ServerCommands::WebSessionToken *cmd = static_cast<IPC::ServerCommands::WebSessionToken *>(command);
        if (cmd->purpose_ == WEB_SESSION_PURPOSE_MANAGE_ACCOUNT)
        {
            Q_EMIT webSessionTokenForManageAccount(cmd->tempSessionToken_);
        }
        else if (cmd->purpose_ == WEB_SESSION_PURPOSE_ADD_EMAIL)
        {
            Q_EMIT webSessionTokenForAddEmail(cmd->tempSessionToken_);
        }
        else if (cmd->purpose_ == WEB_SESSION_PURPOSE_MANAGE_ROBERT_RULES)
        {
            Q_EMIT webSessionTokenForManageRobertRules(cmd->tempSessionToken_);
        }
    }
    else if (command->getStringId() == IPC::ServerCommands::RobertFiltersUpdated::getCommandStringId())
    {
        IPC::ServerCommands::RobertFiltersUpdated *cmd = static_cast<IPC::ServerCommands::RobertFiltersUpdated *>(command);
        Q_EMIT robertFiltersChanged(cmd->success_, cmd->filters_);
    }
    else if (command->getStringId() == IPC::ServerCommands::SetRobertFilterFinished::getCommandStringId())
    {
        IPC::ServerCommands::SetRobertFilterFinished *cmd = static_cast<IPC::ServerCommands::SetRobertFilterFinished *>(command);
        Q_EMIT setRobertFilterResult(cmd->success_);
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
    QString friendlyName = networkInterface.networkOrSsid;

    QVector<types::NetworkInterface> networkListOld = PersistentState::instance().networkWhitelist();
    for (int i = 0; i < networkListOld.size(); i++)
    {
        if (networkListOld[i].networkOrSsid== networkInterface.networkOrSsid)
        {
            friendlyName = networkListOld[i].friendlyName;
            newNetwork = false;
            break;
        }
    }

    if (friendlyName == "") friendlyName = networkInterface.networkOrSsid;
    networkInterface.friendlyName = friendlyName;

    if (networkInterface.networkOrSsid != "") // not a disconnect
    {
        // Add a new network as secured
        if (newNetwork)
        {

#ifdef Q_OS_MAC
            // generate friendly name for MacOS Ethernet
            if (networkInterface.interfaceType == NETWORK_INTERFACE_ETH)
            {
                friendlyName = generateNewFriendlyName();
                networkInterface.friendlyName = friendlyName;

            }
#endif
            types::NetworkInterface newEntry;
            newEntry = networkInterface;
            if (preferences_.isAutoSecureNetworks())
            {
                newEntry.trustType = NETWORK_TRUST_SECURED;
            }
            else
            {
                newEntry.trustType = NETWORK_TRUST_UNSECURED;
            }
            networkListOld << newEntry;
            preferences_.setNetworkWhiteList(networkListOld);
        }

        // GUI-side persistent list holds trustiness
        QVector<types::NetworkInterface> networkList = PersistentState::instance().networkWhitelist();
        types::NetworkInterface foundInterface;
        for (int i = 0; i < networkList.size(); i++)
        {
            if (networkList[i].networkOrSsid == networkInterface.networkOrSsid)
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
                if (preferences_.isAutoConnect())
                {
                    qCDebug(LOG_BASIC) << "Network Whitelisting detected SECURED network -- Connecting..";
                    sendConnect(PersistentState::instance().lastLocation());
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
    IPC::ClientCommands::DetectPacketSize cmd;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendSplitTunneling(const types::SplitTunneling &st)
{
    IPC::ClientCommands::SplitTunneling cmd;
    cmd.splitTunneling_= st;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
    Q_EMIT splitTunnelingStateChanged(st.settings.active);
}

void Backend::sendUpdateWindowInfo(qint32 mainWindowCenterX, qint32 mainWindowCenterY)
{
    IPC::ClientCommands::UpdateWindowInfo cmd;
    cmd.windowCenterX_ = mainWindowCenterX;
    cmd.windowCenterY_ = mainWindowCenterY;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendUpdateVersion(qint64 mainWindowHandle)
{
    IPC::ClientCommands::UpdateVersion cmd;
    cmd.hwnd_ = mainWindowHandle;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::cancelUpdateVersion()
{
    IPC::ClientCommands::UpdateVersion cmd;
    cmd.cancelDownload_ = true;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
    engineServer_->sendCommand(&cmd);
}

void Backend::sendMakeHostsFilesWritableWin()
{
    IPC::ClientCommands::MakeHostsWritableWin cmd;
    //qCDebugMultiline(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
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
    accountInfo_.setLastReset(latestSessionStatus_.getLastResetDate());
    accountInfo_.setPlan(latestSessionStatus_.getTrafficMax());
    accountInfo_.setTrafficUsed(latestSessionStatus_.getTrafficUsed());
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
