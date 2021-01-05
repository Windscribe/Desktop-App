#include "backend.h"

#include "utils/logger.h"
#include "utils/utils.h"
#include "ipc/connection.h"
#include "ipc/protobufcommand.h"
#include "utils/utils.h"
#include "utils/executable_signature/executable_signature.h"
#include "persistentstate.h"
#include <QCoreApplication>

const int PROTOCOL_VERSION = 1;

const int typeIdNetworkInterface = qRegisterMetaType<ProtoTypes::NetworkInterface>("ProtoTypes::NetworkInterface");

Backend::Backend(unsigned int clientId, unsigned long clientPid, const QString &clientName, QObject *parent) : QObject(parent),
    ipcState_(IPC_INIT_STATE),
    bRecoveringState_(false),
    protocolVersion_(PROTOCOL_VERSION),
    clientId_(clientId),
    clientPid_(clientPid),
    clientName_(clientName),
    isSavedApiSettingsExists_(false),
    bLastLoginWithAuthHash_(false),
    process_(NULL),
    connection_(NULL),
    cmdId_(0),
    isFirewallEnabled_(false),
    isExternalConfigMode_(false)
{
    preferences_.loadGuiSettings();

    locationsModel_ = new LocationsModel(this);

    connect(&connectStateHelper_, SIGNAL(connectStateChanged(ProtoTypes::ConnectState)), SIGNAL(connectStateChanged(ProtoTypes::ConnectState)));
    connect(&emergencyConnectStateHelper_, SIGNAL(connectStateChanged(ProtoTypes::ConnectState)), SIGNAL(emergencyConnectStateChanged(ProtoTypes::ConnectState)));
    connect(&firewallStateHelper_, SIGNAL(firewallStateChanged(bool)), SIGNAL(firewallStateChanged(bool)));
}

Backend::~Backend()
{
    SAFE_DELETE(connection_);
    preferences_.saveGuiSettings();
    if (process_)
    {
        process_->waitForFinished(10000);
    }
}

void Backend::init()
{
    qCDebug(LOG_BASIC) << "Backend::init()";

    Q_ASSERT(connection_ == NULL);
    Q_ASSERT(process_ == NULL);

    connection_ = new IPC::Connection();
    connect(dynamic_cast<QObject*>(connection_), SIGNAL(newCommand(IPC::Command *, IPC::IConnection *)), SLOT(onConnectionNewCommand(IPC::Command *, IPC::IConnection *)), Qt::QueuedConnection);
    connect(dynamic_cast<QObject*>(connection_), SIGNAL(stateChanged(int, IPC::IConnection *)), SLOT(onConnectionStateChanged(int, IPC::IConnection *)), Qt::QueuedConnection);

#ifdef WINDSCRIBE_EMBEDDED_ENGINE

#ifdef Q_OS_WIN
    QString engineExePath = QCoreApplication::applicationDirPath() + "/WindscribeEngine.exe";
#else
    QString engineExePath = QCoreApplication::applicationDirPath() + "/../Library/WindscribeEngine.app";
#endif

    qCDebug(LOG_BASIC()) << "Calling Engine at: " << engineExePath;
    if (!ExecutableSignature::verify(engineExePath))
    {
        qCDebug(LOG_BASIC()) << "Engine signature invalid";
        emit initFinished(ProtoTypes::INIT_CLEAN);
        return;
    }

    process_ = new QProcess(this);
    connect(process_, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(onProcessFinished(int,QProcess::ExitStatus)));
    connect(process_, SIGNAL(started()), SLOT(onProcessStarted()));

    process_->setProgram(engineExePath);
    process_->setWorkingDirectory(QCoreApplication::applicationDirPath());
    ipcState_ = IPC_STARTING_PROCESS;
    process_->start();
#else
    ipcState_ = IPC_CONNECTING;
    connectingTimer_.start();
    connection_->connect();
#endif
}

void Backend::basicInit()
{
    qCDebug(LOG_BASIC) << "Backend::basicInit()";

    Q_ASSERT(connection_ == NULL);
    Q_ASSERT(process_ == NULL);

    connection_ = new IPC::Connection();
    connect(dynamic_cast<QObject*>(connection_), SIGNAL(newCommand(IPC::Command *, IPC::IConnection *)), SLOT(onConnectionNewCommand(IPC::Command *, IPC::IConnection *)), Qt::QueuedConnection);
    connect(dynamic_cast<QObject*>(connection_), SIGNAL(stateChanged(int, IPC::IConnection *)), SLOT(onConnectionStateChanged(int, IPC::IConnection *)), Qt::QueuedConnection);

    ipcState_ = IPC_CONNECTING;
    connectingTimer_.start();
    connection_->connect();
}

void Backend::basicClose()
{
    qCDebug(LOG_BASIC) << "Backend::closeConnectionWithoutCleanup()";
    ipcState_ = IPC_DOING_CLEANUP;
    connection_->close();
}

void Backend::cleanup(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart)
{
    qCDebug(LOG_BASIC) << "Backend::cleanup()";

    //Q_ASSERT(isInitFinished());

    if (ipcState_ >= IPC_CONNECTED && ipcState_ < IPC_READY)
    {
        ipcState_ = IPC_FINISHED_STATE;
        connection_->close();
    }
    else if (ipcState_ == IPC_READY)
    {
        IPC::ProtobufCommand<IPCClientCommands::Cleanup> cmd;
        cmd.getProtoObj().set_is_exit_with_restart(isExitWithRestart);
        cmd.getProtoObj().set_is_firewall_checked(isFirewallChecked);
        cmd.getProtoObj().set_is_firewall_always_on(isFirewallAlwaysOn);
        cmd.getProtoObj().set_is_launch_on_start(isLaunchOnStart);
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        ipcState_ = IPC_DOING_CLEANUP;
        connection_->sendCommand(cmd);
    }
}

void Backend::enableBFE_win()
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::EnableBfe_win> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::login(const QString &username, const QString &password, const QString &code2fa)
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
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
        qCDebug(LOG_IPC) << QString::fromStdString(loggedCmd.getDebugString());

        connection_->sendCommand(cmd);
    }
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
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        bLastLoginWithAuthHash_ = true;
        IPC::ProtobufCommand<IPCClientCommands::Login> cmd;
        cmd.getProtoObj().set_auth_hash(authHash.toStdString());
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

QString Backend::getCurrentAuthHash() const
{
    return accountInfo_.authHash();
}

void Backend::loginWithLastLoginSettings()
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::Login> cmd;
        cmd.getProtoObj().set_use_last_login_settings(true);
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

bool Backend::isLastLoginWithAuthHash() const
{
    return bLastLoginWithAuthHash_;
}

void Backend::signOut()
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::SignOut> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::sendConnect(const LocationID &lid)
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        connectStateHelper_.connectClickFromUser();
        IPC::ProtobufCommand<IPCClientCommands::Connect> cmd;
        *cmd.getProtoObj().mutable_locationdid() = lid.toProtobuf();
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::sendDisconnect()
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        connectStateHelper_.disconnectClickFromUser();
        IPC::ProtobufCommand<IPCClientCommands::Disconnect> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
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
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::ForceCliStateUpdate> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::firewallOn(bool updateHelperFirst)
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        if (updateHelperFirst) firewallStateHelper_.firewallOnClickFromGUI();
        IPC::ProtobufCommand<IPCClientCommands::Firewall> cmd;
        cmd.getProtoObj().set_is_enable(true);
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::firewallOff(bool updateHelperFirst)
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        if (updateHelperFirst) firewallStateHelper_.firewallOffClickFromGUI();
        IPC::ProtobufCommand<IPCClientCommands::Firewall> cmd;
        cmd.getProtoObj().set_is_enable(false);
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

bool Backend::isFirewallEnabled()
{
    return firewallStateHelper_.isEnabled();
}

bool Backend::isFirewallAlwaysOn()
{
    return getPreferences()->firewalSettings().mode() == ProtoTypes::FIREWALL_MODE_ALWAYS_ON;
}

void Backend::emergencyConnectClick()
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        emergencyConnectStateHelper_.connectClickFromUser();
        IPC::ProtobufCommand<IPCClientCommands::EmergencyConnect> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::emergencyDisconnectClick()
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        emergencyConnectStateHelper_.disconnectClickFromUser();
        IPC::ProtobufCommand<IPCClientCommands::EmergencyDisconnect> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

bool Backend::isEmergencyDisconnected()
{
    return emergencyConnectStateHelper_.isDisconnected();
}

void Backend::startWifiSharing(const QString &ssid, const QString &password)
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::StartWifiSharing> cmd;
        cmd.getProtoObj().set_ssid(ssid.toStdString());
        cmd.getProtoObj().set_password(password.toStdString());
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::stopWifiSharing()
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::StopWifiSharing> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::startProxySharing(ProtoTypes::ProxySharingMode proxySharingMode)
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::StartProxySharing> cmd;
        cmd.getProtoObj().set_sharing_mode(proxySharingMode);
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::stopProxySharing()
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::StopProxySharing> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::setIPv6StateInOS(bool bEnabled)
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::SetIpv6StateInOS> cmd;
        cmd.getProtoObj().set_is_enabled(bEnabled);
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::getAndUpdateIPv6StateInOS()
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::GetIpv6StateInOS> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::gotoCustomOvpnConfigMode()
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::GotoCustomOvpnConfigMode> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::recordInstall()
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::RecordInstall> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::sendConfirmEmail()
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::SendConfirmEmail> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::sendDebugLog()
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::SendDebugLog> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::speedRating(int rating, const QString &localExternalIp)
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::SpeedRating> cmd;
        cmd.getProtoObj().set_rating(rating);
        cmd.getProtoObj().set_local_external_ip(localExternalIp.toStdString());
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::setBlockConnect(bool isBlockConnect)
{
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::SetBlockConnect> cmd;
        cmd.getProtoObj().set_is_block_connect(isBlockConnect);
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::clearCredentials()
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::ClearCredentials> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

bool Backend::isInitFinished() const
{
    return ipcState_ == IPC_READY;
}

bool Backend::isAppCanClose() const
{
    return ipcState_ == IPC_INIT_STATE || ipcState_ == IPC_STARTING_PROCESS || ipcState_ == IPC_CONNECTING;
}

void Backend::continueWithCredentialsForOvpnConfig(const QString &username, const QString &password, bool bSave)
{
    Q_ASSERT(isInitFinished());
    if (isInitFinished())
    {
        qCDebug(LOG_BASIC) << "Backend::continueWithCredentialsForOvpnConfig()";
        IPC::ProtobufCommand<IPCClientCommands::ContinueWithCredentialsForOvpnConfig> cmd;
        cmd.getProtoObj().set_username(username.toStdString());
        cmd.getProtoObj().set_password(password.toStdString());
        cmd.getProtoObj().set_is_save(bSave);
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::sendEngineSettingsIfChanged()
{
    if(!google::protobuf::util::MessageDifferencer::Equals(preferences_.getEngineSettings(), latestEngineSettings_))
    {
        Q_ASSERT(isInitFinished());
        if (isInitFinished())
        {
            qCDebug(LOG_BASIC) << "Engine settings changed, sent to engine";
            latestEngineSettings_ = preferences_.getEngineSettings();
            IPC::ProtobufCommand<IPCClientCommands::SetSettings> cmd;
            *cmd.getProtoObj().mutable_enginesettings() = latestEngineSettings_;
            qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
            connection_->sendCommand(cmd);
        }
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
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::ApplicationActivated> cmd;
        cmd.getProtoObj().set_is_activated(true);
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::applicationDeactivated()
{
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::ApplicationActivated> cmd;
        cmd.getProtoObj().set_is_activated(false);
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

const ProtoTypes::SessionStatus &Backend::getSessionStatus() const
{
    return latestSessionStatus_;
}

void Backend::onProcessStarted()
{
    Q_ASSERT(ipcState_ == IPC_STARTING_PROCESS);
    if (connection_)
    {
        qCDebug(LOG_BASIC) << "Backend::onProcessStarted()";
        ipcState_ = IPC_CONNECTING;
        connectingTimer_.start();
        connection_->connect();
    }
}

void Backend::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (sender() != process_)
    {
        return;
    }
    qCDebug(LOG_BASIC) << "Backend::onProcessFinished()" << exitCode << (int)exitStatus;
    if (ipcState_ == IPC_STARTING_PROCESS || ipcState_ == IPC_CONNECTING || ipcState_ == IPC_CONNECTED)
    {
        ipcState_ = IPC_INIT_STATE;
        emit initFinished(ProtoTypes::INIT_CLEAN);
    }
    else if (ipcState_ == IPC_DOING_CLEANUP || ipcState_ == IPC_FINISHED_STATE)
    {
        ipcState_ = IPC_INIT_STATE;
        emit cleanupFinished();
    }
    else if (ipcState_ == IPC_READY)
    {
        if (!bRecoveringState_)
        {
            qCDebug(LOG_BASIC) << "Start engine recovery.";
            engineCrashedDoRecovery();
        }
    }
    else
    {
        Q_ASSERT(false);
    }
}

void Backend::onConnectionNewCommand(IPC::Command *command, IPC::IConnection * /*connection*/)
{
    QScopedPointer<IPC::Command> pCommand(command);

    if (command->getStringId() == IPCServerCommands::AuthReply::descriptor()->full_name())
    {
        Q_ASSERT(ipcState_ == IPC_CONNECTED);
        if (connection_)
        {
            ipcState_ = IPC_INIT_SENDING;
            IPC::ProtobufCommand<IPCClientCommands::Init> cmd;
            qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
            connection_->sendCommand(cmd);
        }
    }
    else if (command->getStringId() == IPCServerCommands::InitFinished::descriptor()->full_name())
    {
        // if (ipcState_ != IPC_READY) // safe? -- prevent triggering GUI initFinished when Engine Init is broadcast as a result of CLI init
        {
            IPC::ProtobufCommand<IPCServerCommands::InitFinished> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::InitFinished> *>(command);
            if (cmd->getProtoObj().init_state() == ProtoTypes::INIT_SUCCESS)
            {
                latestEngineSettings_ = cmd->getProtoObj().engine_settings();
                preferences_.setEngineSettings(latestEngineSettings_);
                getOpenVpnVersionsFromInitCommand(cmd->getProtoObj());

                // WiFi sharing supported state
                if (cmd->getProtoObj().has_is_wifi_sharing_supported())
                {
                    preferencesHelper_.setWifiSharingSupported(cmd->getProtoObj().is_wifi_sharing_supported());
                }

                isSavedApiSettingsExists_ = cmd->getProtoObj().is_saved_api_settings_exists();
                accountInfo_.setAuthHash(QString::fromStdString(cmd->getProtoObj().auth_hash()));
            }
            ipcState_ = IPC_READY;
            bRecoveringState_ = false;
            emit initFinished(cmd->getProtoObj().init_state());
        }
    }
    else if (command->getStringId() == IPCServerCommands::FirewallStateChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::FirewallStateChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::FirewallStateChanged> *>(command);
        qCDebug(LOG_IPC) << QString::fromStdString(cmd->getDebugString());
        firewallStateHelper_.setFirewallStateFromEngine(cmd->getProtoObj().is_firewall_enabled());
    }
    else if (command->getStringId() == IPCServerCommands::LoginFinished::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::LoginFinished> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::LoginFinished> *>(command);

        if (cmd->getProtoObj().has_auth_hash())
        {
            accountInfo_.setAuthHash(QString::fromStdString(cmd->getProtoObj().auth_hash()));
        }
        if (cmd->getProtoObj().has_array_port_map())
        {
            preferencesHelper_.setPortMap(cmd->getProtoObj().array_port_map());
        }

        emit loginFinished(cmd->getProtoObj().is_login_from_saved_settings());
    }
    else if (command->getStringId() == IPCServerCommands::LoginStepMessage::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::LoginStepMessage> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::LoginStepMessage> *>(command);
        emit loginStepMessage(cmd->getProtoObj().message());
    }
    else if (command->getStringId() == IPCServerCommands::LoginError::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::LoginError> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::LoginError> *>(command);
        emit loginError(cmd->getProtoObj().error());
    }
    else if (command->getStringId() == IPCServerCommands::SessionStatusUpdated::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::SessionStatusUpdated> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::SessionStatusUpdated> *>(command);
        latestSessionStatus_ = cmd->getProtoObj().session_status();
        locationsModel_->setFreeSessionStatus(!latestSessionStatus_.is_premium());
        updateAccountInfo();
        emit sessionStatusChanged(latestSessionStatus_);
    }
    else if (command->getStringId() == IPCServerCommands::LocationsUpdated::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::LocationsUpdated> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::LocationsUpdated> *>(command);
        locationsModel_->updateApiLocations(cmd->getProtoObj().best_location(), cmd->getProtoObj().locations());
        emit locationsUpdated();
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
        emit locationsUpdated();
    }
    else if (command->getStringId() == IPCServerCommands::LocationSpeedChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::LocationSpeedChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::LocationSpeedChanged> *>(command);
        locationsModel_->changeConnectionSpeed(LocationID::createFromProtoBuf(cmd->getProtoObj().id()), (int)cmd->getProtoObj().pingtime());
    }
    else if (command->getStringId() == IPCServerCommands::ConnectStateChanged::descriptor()->full_name())
    {
        qCDebug(LOG_IPC) << QString::fromStdString(command->getDebugString());
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
        emit proxySharingInfoChanged(cmd->getProtoObj().proxy_sharing_info());
    }
    else if (command->getStringId() == IPCServerCommands::WifiSharingInfoChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::WifiSharingInfoChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::WifiSharingInfoChanged> *>(command);
        emit wifiSharingInfoChanged(cmd->getProtoObj().wifi_sharing_info());
    }
    else if (command->getStringId() == IPCServerCommands::SignOutFinished::descriptor()->full_name())
    {
        emit signOutFinished();
    }
    else if (command->getStringId() == IPCServerCommands::NotificationsUpdated::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::NotificationsUpdated> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::NotificationsUpdated> *>(command);
        emit notificationsChanged(cmd->getProtoObj().array_notifications());
    }
    else if (command->getStringId() == IPCServerCommands::CheckUpdateInfoUpdated::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::CheckUpdateInfoUpdated> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::CheckUpdateInfoUpdated> *>(command);
        emit checkUpdateChanged(cmd->getProtoObj().check_update_info());
    }
    else if (command->getStringId() == IPCServerCommands::MyIpUpdated::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::MyIpUpdated> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::MyIpUpdated> *>(command);
        emit myIpChanged(QString::fromStdString(cmd->getProtoObj().my_ip_info().ip()), cmd->getProtoObj().my_ip_info().is_disconnected_state());
    }
    else if (command->getStringId() == IPCServerCommands::StatisticsUpdated::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::StatisticsUpdated> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::StatisticsUpdated> *>(command);
        emit statisticsUpdated(cmd->getProtoObj().bytes_in(), cmd->getProtoObj().bytes_out(), cmd->getProtoObj().is_total_bytes());
    }
    else if (command->getStringId() == IPCServerCommands::RequestCredentialsForOvpnConfig::descriptor()->full_name())
    {
        emit requestCustomOvpnConfigCredentials();
    }
    else if (command->getStringId() == IPCServerCommands::DebugLogResult::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::DebugLogResult> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::DebugLogResult> *>(command);
        emit debugLogResult(cmd->getProtoObj().success());
    }
    else if (command->getStringId() == IPCServerCommands::ConfirmEmailResult::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::ConfirmEmailResult> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::ConfirmEmailResult> *>(command);
        emit confirmEmailResult(cmd->getProtoObj().success());
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
        emit gotoCustomOvpnConfigModeFinished();
    }
    else if (command->getStringId() == IPCServerCommands::EngineSettingsChanged::descriptor()->full_name())
    {
        qCDebug(LOG_IPC) << QString::fromStdString(command->getDebugString());
        latestEngineSettings_ = static_cast<IPC::ProtobufCommand<IPCServerCommands::EngineSettingsChanged> *>(command)->getProtoObj().enginesettings();
        preferences_.setEngineSettings(latestEngineSettings_);
    }
    else if (command->getStringId() == IPCServerCommands::CleanupFinished::descriptor()->full_name())
    {
        ipcState_ = IPC_FINISHED_STATE;
        if (connection_)
        {
            connection_->close();
        }
        //emit cleanupFinished();
    }
    else if (command->getStringId() == IPCServerCommands::NetworkChanged::descriptor()->full_name())
    {
        qCDebug(LOG_IPC) << QString::fromStdString(command->getDebugString());
        IPC::ProtobufCommand<IPCServerCommands::NetworkChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::NetworkChanged> *>(command);

        ProtoTypes::NetworkInterface networkInterface;
        networkInterface.set_interface_index(cmd->getProtoObj().network_interface().interface_index());
        networkInterface.set_interface_name(cmd->getProtoObj().network_interface().interface_name());
        networkInterface.set_interface_guid(cmd->getProtoObj().network_interface().interface_guid());
        networkInterface.set_network_or_ssid(cmd->getProtoObj().network_interface().network_or_ssid());
        networkInterface.set_interface_type(cmd->getProtoObj().network_interface().interface_type());
        networkInterface.set_trust_type(cmd->getProtoObj().network_interface().trust_type());
        networkInterface.set_active(cmd->getProtoObj().network_interface().active());
        networkInterface.set_friendly_name(cmd->getProtoObj().network_interface().friendly_name());
        networkInterface.set_requested(cmd->getProtoObj().network_interface().requested());
        networkInterface.set_metric(cmd->getProtoObj().network_interface().metric());
        networkInterface.set_physical_address(cmd->getProtoObj().network_interface().physical_address());
        networkInterface.set_mtu(cmd->getProtoObj().network_interface().mtu());
        handleNetworkChange(networkInterface);
    }
    else if (command->getStringId() == IPCServerCommands::SessionDeleted::descriptor()->full_name())
    {
        emit sessionDeleted();
    }
    else if (command->getStringId() == IPCServerCommands::TestTunnelResult::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::TestTunnelResult> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::TestTunnelResult> *>(command);
        emit testTunnelResult(cmd->getProtoObj().success());
    }
    else if (command->getStringId() == IPCServerCommands::LostConnectionToHelper::descriptor()->full_name())
    {
        emit lostConnectionToHelper();
    }
    else if (command->getStringId() == IPCServerCommands::HighCpuUsage::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::HighCpuUsage> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::HighCpuUsage> *>(command);
        QStringList list;
        for (int i = 0; i < cmd->getProtoObj().processes_size(); ++i)
        {
            list << QString::fromStdString(cmd->getProtoObj().processes(i));
        }
        emit highCpuUsage(list);
    }
    else if (command->getStringId() == IPCServerCommands::UserWarning::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::UserWarning> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::UserWarning> *>(command);
        emit userWarning(cmd->getProtoObj().type());
    }
    else if (command->getStringId() == IPCServerCommands::InternetConnectivityChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::InternetConnectivityChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::InternetConnectivityChanged> *>(command);
        emit internetConnectivityChanged(cmd->getProtoObj().connectivity());
    }
    else if (command->getStringId() == IPCServerCommands::ProtocolPortChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::ProtocolPortChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::ProtocolPortChanged> *>(command);
        emit protocolPortChanged(cmd->getProtoObj().protocol(), cmd->getProtoObj().port());
    }
    else if (command->getStringId() == IPCServerCommands::PacketSizeDetectionState::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::PacketSizeDetectionState> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::PacketSizeDetectionState> *>(command);
        emit packetSizeDetectionStateChanged(cmd->getProtoObj().on(), cmd->getProtoObj().is_error());
    }
    else if (command->getStringId() == IPCServerCommands::UpdateVersionChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<IPCServerCommands::UpdateVersionChanged> *cmd = static_cast<IPC::ProtobufCommand<IPCServerCommands::UpdateVersionChanged> *>(command);
        emit updateVersionChanged(cmd->getProtoObj().progress(), cmd->getProtoObj().state(), cmd->getProtoObj().error());
    }
}

void Backend::onConnectionStateChanged(int state, IPC::IConnection * /*connection*/)
{
    if (state == IPC::CONNECTION_CONNECTED)
    {
        qCDebug(LOG_BASIC) << "Connected to engine server";

        ipcState_ = IPC_CONNECTED;

        IPC::ProtobufCommand<IPCClientCommands::ClientAuth> cmd;
        cmd.getProtoObj().set_protocol_version(protocolVersion_);
        cmd.getProtoObj().set_client_id(clientId_);
        cmd.getProtoObj().set_pid(clientPid_);
        cmd.getProtoObj().set_name(clientName_.toStdString());
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
    else if (state == IPC::CONNECTION_DISCONNECTED)
    {
        qCDebug(LOG_BASIC) << "Disconnected from engine server";
        if (ipcState_ == IPC_DOING_CLEANUP)
        {
            ipcState_ = IPC_INIT_STATE;
            emit cleanupFinished();
        }
        else if (ipcState_ == IPC_FINISHED_STATE)
        {
#ifdef WINDSCRIBE_EMBEDDED_ENGINE
            // nothing todo (waiting for process finish and emit cleanupFinished() there
#else
            ipcState_ = IPC_INIT_STATE;
            emit cleanupFinished();
#endif
        }
        else if (ipcState_ != IPC_READY)
        {
            emit initFinished(ProtoTypes::INIT_CLEAN);
        }
        else if (ipcState_ == IPC_READY)
        {
            if (!bRecoveringState_)
            {
                qCDebug(LOG_BASIC) << "Start engine recovery.";
                engineCrashedDoRecovery();
            }
        }

    }
    else if (state == IPC::CONNECTION_ERROR)
    {
        if (ipcState_ == IPC_CONNECTING)
        {
            if (connectingTimer_.elapsed() > MAX_CONNECTING_TIME)
            {
                qCDebug(LOG_BASIC) << "Connection error to engine server";
                if (connection_)
                {
                    connection_->close();
                }
                emit initFinished(ProtoTypes::INIT_CLEAN);
            }
            else
            {
                // try connect again
				// Delay necessary so that Engine process will acutally start running on low resource systems
                QTimer::singleShot(100, [this](){
                    connection_->connect();
                });
            }
        }
        else if (ipcState_ >= IPC_CONNECTED)
        {
            if (!bRecoveringState_)
            {
                qCDebug(LOG_BASIC) << "Connection error to engine server. Start engine recovery.";
                engineCrashedDoRecovery();
            }
        }
    }
}

// Assumes that duplicate network filtering occurs on Engine side
void Backend::handleNetworkChange(ProtoTypes::NetworkInterface networkInterface)
{
    bool newNetwork = true;

    // find or assign friendly name before checking is network is the same as current network
    QString friendlyName = QString::fromStdString(networkInterface.network_or_ssid());

    ProtoTypes::NetworkWhiteList networkListOld = PersistentState::instance().networkWhitelist();
    for (int i = 0; i < networkListOld.networks_size(); i++)
    {
        if (networkListOld.networks(i).network_or_ssid() == networkInterface.network_or_ssid())
        {
            friendlyName = QString::fromStdString(networkListOld.networks(i).friendly_name());
            newNetwork = false;
            break;
        }
    }

    if (friendlyName == "") friendlyName = QString::fromStdString(networkInterface.network_or_ssid());
    networkInterface.set_friendly_name(friendlyName.toStdString().c_str());

    if (QString::fromStdString(networkInterface.network_or_ssid()) != "") // not a disconnect
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
            ProtoTypes::NetworkInterface newEntry;
            newEntry.CopyFrom(networkInterface);
            *networkListOld.add_networks() = newEntry;

            preferences_.setNetworkWhiteList(networkListOld);
        }

        // GUI-side persistent list holds trustiness
        ProtoTypes::NetworkWhiteList networkList = PersistentState::instance().networkWhitelist();
        ProtoTypes::NetworkInterface foundInterface;
        for (int i = 0; i < networkList.networks_size(); i++)
        {
            if (networkList.networks(i).network_or_ssid() == networkInterface.network_or_ssid())
            {
                foundInterface = networkList.networks(i);
                break;
            }
        }

        if (!Utils::sameNetworkInterface(networkInterface, currentNetworkInterface_)) // actual network change -- prevents brief/rare network loss during CONNECTING from triggering network change
        {
            // disconnect VPN on an unsecured network -- connect VPN on a secured network if auto-connect is on
            if (foundInterface.trust_type() == ProtoTypes::NETWORK_UNSECURED)
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
        ProtoTypes::NetworkInterface protoInterface;
        protoInterface.CopyFrom(networkInterface);
        protoInterface.set_trust_type(foundInterface.trust_type());
        emit networkChanged(protoInterface);

    }
    else
    {
        // inform UI no network
        emit networkChanged(networkInterface);
    }
}

void Backend::cycleMacAddress()
{
    QString macAddress = Utils::generateRandomMacAddress(); // TODO: move generation into Engine (BEWARE: mac change only occurs on preferences closing and when manual MAC cycled)

    ProtoTypes::MacAddrSpoofing mas = preferences_.macAddrSpoofing();
    mas.set_mac_address(macAddress.toStdString());
    preferences_.setMacAddrSpoofing(mas);

    sendEngineSettingsIfChanged(); //  force EngineSettings update on manual MAC cycle button press
}

void Backend::sendDetectPacketSize()
{
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::DetectPacketSize> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::sendSplitTunneling(ProtoTypes::SplitTunneling st)
{
    if (isInitFinished())
    {
         IPC::ProtobufCommand<IPCClientCommands::SplitTunneling> cmd;
         *cmd.getProtoObj().mutable_split_tunneling() = st;
         qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
         connection_->sendCommand(cmd);
         emit splitTunnelingStateChanged(st.settings().active());
    }
}

void Backend::sendUpdateVersion()
{
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::UpdateVersion> cmd;
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

void Backend::cancelUpdateVersion()
{
    if (isInitFinished())
    {
        IPC::ProtobufCommand<IPCClientCommands::UpdateVersion> cmd;
        cmd.getProtoObj().set_cancel_download(true);
        qCDebug(LOG_IPC) << QString::fromStdString(cmd.getDebugString());
        connection_->sendCommand(cmd);
    }
}

QString Backend::generateNewFriendlyName()
{
    QList<QString> friendlyNames;
    ProtoTypes::NetworkWhiteList whiteList = preferences_.networkWhiteList();
    for (int i = 0; i < whiteList.networks_size(); i++)
    {
        ProtoTypes::NetworkInterface network = whiteList.networks(i);
        friendlyNames.append(QString::fromStdString(network.friendly_name()));
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
    accountInfo_.setEmail(QString::fromStdString(latestSessionStatus_.email()));
    accountInfo_.setNeedConfirmEmail(latestSessionStatus_.email_status() == 0);
    accountInfo_.setUsername(QString::fromStdString(latestSessionStatus_.username()));
    accountInfo_.setExpireDate(QString::fromStdString(latestSessionStatus_.premium_expire_date()));
    accountInfo_.setPlan(latestSessionStatus_.traffic_max());
}

void Backend::getOpenVpnVersionsFromInitCommand(const IPCServerCommands::InitFinished &cmd)
{
    // OpenVpn versions
    if (cmd.available_openvpn_versions_size() > 0)
    {
        QStringList list;
        for (int i = 0; i < cmd.available_openvpn_versions_size(); ++i)
        {
            list << QString::fromStdString(cmd.available_openvpn_versions(i));
        }

        preferencesHelper_.setAvailableOpenVpnVersions(list);
    }
}

void Backend::engineCrashedDoRecovery()
{
    bRecoveringState_ = true;
    emit engineCrash();

    ipcState_ = IPC_INIT_STATE;
    SAFE_DELETE_LATER(process_);
    SAFE_DELETE(connection_);

    // kill WindscribeEngine.exe process with external command
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start("taskkill", QStringList() << "/f" << "/t" << "/im" << "WindscribeEngine.exe");
    process.waitForFinished(-1);
    QString strAnswer = QString::fromStdString((const char *)process.readAll().data()).toLower();
    qCDebug(LOG_BASIC) << "Backend::engineCrashedDoRecovery, taskkill output:" << strAnswer;

    ProtoTypes::ConnectState cs;
    cs.set_connect_state_type(ProtoTypes::DISCONNECTED);
    connectStateHelper_.setConnectStateFromEngine(cs);
    emergencyConnectStateHelper_.setConnectStateFromEngine(cs);

    init();
}
