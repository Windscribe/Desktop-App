#include "localipcserver.h"

#include <memory>

#include "backend/persistentstate.h"
#include "ipc/server.h"
#include "locations/locationsmodel_roles.h"
#include "locations/model/locationsmodel.h"
#include "utils/hydraprofile.h"
#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"

LocalIPCServer::LocalIPCServer(Backend *backend, QObject *parent) : QObject(parent)
  , backend_(backend), updateError_(UPDATE_VERSION_ERROR_NO_ERROR), updateProgress_(0), disconnectedByKeyLimit_(false)
{
    connect(backend_, &Backend::checkUpdateChanged, this, &LocalIPCServer::onBackendCheckUpdateChanged);
    connect(backend_, &Backend::connectStateChanged, this, &LocalIPCServer::onBackendConnectStateChanged);
    connect(backend_, &Backend::internetConnectivityChanged, this, &LocalIPCServer::onBackendInternetConnectivityChanged);
    connect(backend_, &Backend::captchaRequired, this, &LocalIPCServer::onBackendCaptchaRequired);
    connect(backend_, &Backend::loginError, this, &LocalIPCServer::onBackendLoginError);
    connect(backend_, &Backend::loginFinished, this, &LocalIPCServer::onBackendLoginFinished);
    connect(backend_, &Backend::logoutFinished, this, &LocalIPCServer::onBackendLogoutFinished);
    connect(backend_, &Backend::protocolPortChanged, this, &LocalIPCServer::onBackendProtocolPortChanged);
    connect(backend_, &Backend::testTunnelResult, this, &LocalIPCServer::onBackendTestTunnelResult);
    connect(backend_, &Backend::updateDownloaded, this, &LocalIPCServer::onBackendUpdateDownloaded);
    connect(backend_, &Backend::updateVersionChanged, this, &LocalIPCServer::onBackendUpdateVersionChanged);
    connect(backend_, &Backend::connectionIdChanged, this, &LocalIPCServer::onBackendConnectionIdChanged);
    connect(backend_, &Backend::bridgeApiAvailabilityChanged, this, &LocalIPCServer::onBackendBridgeApiAvailabilityChanged);
    connect(backend_, &Backend::ipRotateResult, this, &LocalIPCServer::onBackendIpRotateResult);
    connect(backend_, &Backend::debugLogResult, this, &LocalIPCServer::onBackendDebugLogResult);
    connect(backend_, &Backend::myIpChanged, this, &LocalIPCServer::onBackendMyIpChanged);

    myIp_ = PersistentState::instance().lastExternalIp();

    connect(backend_->locationsModelManager(), &gui_locations::LocationsModelManager::deviceNameChanged, this, &LocalIPCServer::onLocationsModelManagerDeviceNameChanged);
}

LocalIPCServer::~LocalIPCServer()
{
    for (IPC::Connection * connection : std::as_const(connections_)) {
        connection->close();
        delete connection;
    }
    connections_.clear();
    SAFE_DELETE(server_);
    qCDebug(LOG_CLI_IPC) << "IPC server for CLI stopped";
}

void LocalIPCServer::start()
{
    WS_ASSERT(server_ == nullptr);
    server_ = new IPC::Server();
    connect(server_, &IPC::Server::newConnection, this, &LocalIPCServer::onServerCallbackAcceptFunction);

    if (!server_->start()) {
        qCCritical(LOG_CLI_IPC) << "Can't start IPC server for CLI";
    } else {
        qCDebug(LOG_CLI_IPC) << "IPC server for CLI started";
    }
}

void LocalIPCServer::sendLocations(IPC::Connection *connection, const QList<IPC::CliCommands::IpcLocation> &list, IPC::CliCommands::LocationType type)
{
    IPC::CliCommands::LocationsList cmd;
    cmd.locations_ = list;
    if (type == IPC::CliCommands::LocationType::kStaticIp) {
        cmd.deviceName_ = deviceName_;
    }
    reply(connection, cmd);
}

QList<IPC::CliCommands::IpcLocation> LocalIPCServer::locationsList(IPC::CliCommands::LocationType type) const
{
    QList<IPC::CliCommands::IpcLocation> locations;

    if (type == IPC::CliCommands::LocationType::kRegular) {

        for (int i = 0; i < backend_->locationsModelManager()->sortedLocationsProxyModel()->rowCount(); i++) {
            QModelIndex miCountry = backend_->locationsModelManager()->sortedLocationsProxyModel()->index(i, 0);
            LocationID lid = qvariant_cast<LocationID>(miCountry.data(gui_locations::kLocationId));
            QString countryName = miCountry.data(gui_locations::kName).toString();

            if (lid.isBestLocation()) {
                // The model holds the name translated to the GUI language; the CLI output is English-only.
                QString nickName = miCountry.data(gui_locations::kNick).toString();
                IPC::CliCommands::IpcLocation location(gui_locations::LocationsModel::BEST_LOCATION_NAME, "", nickName, 0);
                if (miCountry.data(gui_locations::kIs10Gbps).toBool()) {
                    location.flags |= (int)IPC::CliCommands::LocationFlags::k10Gbps;
                }
                locations << location;
                continue;
            }

            for (int j = 0; j < miCountry.model()->rowCount(miCountry); j++) {
                QModelIndex miCity = miCountry.model()->index(j, 0, miCountry);
                QString cityName = miCity.data(gui_locations::kName).toString();
                QString nickName = miCity.data(gui_locations::kNick).toString();

                IPC::CliCommands::IpcLocation location(countryName, cityName, nickName, 0);
                if (miCity.data(gui_locations::kIsDisabled).toBool()) {
                    location.flags |= (int)IPC::CliCommands::LocationFlags::kDisabled;
                }
                if (miCity.data(gui_locations::kIsShowAsPremium).toBool()) {
                    location.flags |= (int)IPC::CliCommands::LocationFlags::kPremium;
                }
                if (miCity.data(gui_locations::kIs10Gbps).toBool()) {
                    location.flags |= (int)IPC::CliCommands::LocationFlags::k10Gbps;
                }

                locations << location;
            }
        }
    } else if (type == IPC::CliCommands::LocationType::kStaticIp) {
        for (int i = 0; i < backend_->locationsModelManager()->staticIpsProxyModel()->rowCount(); i++) {
            QModelIndex miStatic = backend_->locationsModelManager()->staticIpsProxyModel()->index(i, 0);
            locations << IPC::CliCommands::IpcLocation(miStatic.data(gui_locations::kName).toString(),
                                                       "",
                                                       miStatic.data(gui_locations::kNick).toString(),
                                                       "",
                                                       0);
        }
    } else if (type == IPC::CliCommands::LocationType::kFavourite) {
        for (int i = 0; i < backend_->locationsModelManager()->favoriteCitiesProxyModel()->rowCount(); i++) {
            QModelIndex miFavorite = backend_->locationsModelManager()->favoriteCitiesProxyModel()->index(i, 0);
            QString cityName = miFavorite.data(gui_locations::kName).toString();
            QString nickName = miFavorite.data(gui_locations::kNick).toString();

            // Get pinned IP if available
            QString pinnedIp;
            QVariantList pinnedData = miFavorite.data(gui_locations::kPinnedIp).toList();
            if (pinnedData.size() == 2) {
                pinnedIp = pinnedData[1].toString();  // IP is the second element
            }

            int flags = 0;
            if (miFavorite.data(gui_locations::kIsDisabled).toBool()) {
                flags |= (int)IPC::CliCommands::LocationFlags::kDisabled;
            }
            if (miFavorite.data(gui_locations::kIsShowAsPremium).toBool()) {
                flags |= (int)IPC::CliCommands::LocationFlags::kPremium;
            }
            if (miFavorite.data(gui_locations::kIs10Gbps).toBool()) {
                flags |= (int)IPC::CliCommands::LocationFlags::k10Gbps;
            }

            locations << IPC::CliCommands::IpcLocation("", cityName, nickName, pinnedIp, flags);
        }
    } else {
        WS_ASSERT(false);
    }

    return locations;
}

void LocalIPCServer::onServerCallbackAcceptFunction(IPC::Connection *connection)
{
    qCDebug(LOG_IPC) << "Client connected:" << connection;

    connections_.append(connection);

    connect(connection, &IPC::Connection::newCommand, this, &LocalIPCServer::onConnectionCommandCallback);
    connect(connection, &IPC::Connection::stateChanged, this, &LocalIPCServer::onConnectionStateCallback);
}

void LocalIPCServer::onConnectionCommandCallback(IPC::Command *command, IPC::Connection *connection)
{
    std::unique_ptr<IPC::Command> owner(command);

    if (command->getStringId() ==IPC::CliCommands::ShowLocations::getCommandStringId()) {
        IPC::CliCommands::ShowLocations *cmd = static_cast<IPC::CliCommands::ShowLocations *>(command);
        sendLocations(connection, locationsList(cmd->locationType_), cmd->locationType_);
        return;
    } else if (command->getStringId() == IPC::CliCommands::GetState::getCommandStringId()) {
        sendState(connection);
        return;
    } else if (command->getStringId() == IPC::CliCommands::Connect::getCommandStringId()) {
        IPC::CliCommands::Connect *cmd = static_cast<IPC::CliCommands::Connect *>(command);
        IPC::CliCommands::LocationType type = cmd->locationType_;
        QString locationStr = cmd->location_;
        types::Protocol protocol = types::Protocol::fromString(cmd->protocol_);
        uint port = cmd->port_;
        LocationID lid;

        if (type == IPC::CliCommands::LocationType::kStaticIp) {
            lid = backend_->locationsModelManager()->findStaticIpLocation(locationStr);
        } else if (locationStr.isEmpty()) {
            lid = PersistentState::instance().lastLocation();
            if (!lid.isValid()) {
                lid = backend_->locationsModelManager()->getBestLocationId();
            }
        } else if (locationStr == "best") {
            lid = backend_->locationsModelManager()->getBestLocationId();
        } else {
            lid = backend_->locationsModelManager()->findLocationByFilter(locationStr);
        }

        if (lid.isValid()) {
            emit connectToLocation(lid, protocol, port);
        } else if (backend_->isDisconnected()) {
            // Already disconnected, just set the connect error
            connectState_.connectError = ConnectError::kLocationUnavailable;
        } else {
            invalidLocation_ = true;
            backend_->sendDisconnect();
        }
    } else if (command->getStringId() == IPC::CliCommands::Disconnect::getCommandStringId()) {
        if (!backend_->isDisconnected()) {
            backend_->sendDisconnect();
        }
    } else if (command->getStringId() == IPC::CliCommands::SetIgnoreSslErrors::getCommandStringId()) {
        IPC::CliCommands::SetIgnoreSslErrors *cmd = static_cast<IPC::CliCommands::SetIgnoreSslErrors *>(command);
        if (cmd->isEnable_ && lastLoginError_ != wsnet::LoginResult::kSslError) {
            IPC::CliCommands::Acknowledge ack;
            ack.code_ = 1;
            ack.message_ = "Ignoring SSL errors is not available.";
            sendCommand(ack);
            return;
        }
        emit setIgnoreSslErrors(cmd->isEnable_);
    } else if (command->getStringId() == IPC::CliCommands::SetHydraProfile::getCommandStringId()) {
        const auto *cmd = static_cast<IPC::CliCommands::SetHydraProfile *>(command);
        const auto before = HydraProfile::load();
        const IPC::CliCommands::Acknowledge ack = hydraProfileAnswer(*cmd);
        // Only the asker: another CLI waiting on its own command would take this for its acknowledge.
        connection->sendCommand(ack);
        if (ack.code_ == 0 && cmd->isValid_ && (cmd->remove_ || !cmd->json_.isEmpty())) {
            emit hydraProfileChanged();
            backend_->reconnectIfHydraFrontChanged(before, HydraProfile::load());
        }
        return;
    } else if (command->getStringId() == IPC::CliCommands::Firewall::getCommandStringId()) {
        IPC::CliCommands::Firewall *cmd = static_cast<IPC::CliCommands::Firewall *>(command);
        if (cmd->isEnable_ && !backend_->isFirewallEnabled()) {
            backend_->firewallOn();
        } else if (!cmd->isEnable_ && backend_->isFirewallEnabled() && !backend_->isFirewallAlwaysOn()) {
            backend_->firewallOff();
        }
    } else if (command->getStringId() == IPC::CliCommands::Login::getCommandStringId()) {
        isCaptchaRequired_ = false;
        asciiArt_.clear();
        IPC::CliCommands::Login *cmd = static_cast<IPC::CliCommands::Login *>(command);
        if (!cmd->captchaSolution_.isEmpty()) {
            backend_->continueWithCaptcha(cmd->captchaSolution_, std::vector<float>(), std::vector<float>());
        } else if (backend_->currentLoginState() == LOGIN_STATE_LOGGED_OUT || backend_->currentLoginState() == LOGIN_STATE_LOGIN_ERROR) {
            emit attemptLogin(cmd->username_, cmd->password_, cmd->code2fa_);
        }
    } else if (command->getStringId() == IPC::CliCommands::Logout::getCommandStringId()) {
        if (backend_->currentLoginState() != LOGIN_STATE_LOGGED_OUT) {
            IPC::CliCommands::Logout *cmd = static_cast<IPC::CliCommands::Logout *>(command);
            backend_->logout(cmd->isKeepFirewallOn_);
        }
    } else if (command->getStringId() == IPC::CliCommands::SendLogs::getCommandStringId()) {
        awaitingDebugLogResult_ = true;
        backend_->sendDebugLog();
        return;
    } else if (command->getStringId() == IPC::CliCommands::Update::getCommandStringId()) {
#ifdef CLI_ONLY
        const bool isInstallerStarted = false;
#else
        // The app runs the installer itself; asking again once it has would start a second download and installer.
        const bool isInstallerStarted = updateState_ == UPDATE_VERSION_STATE_RUNNING
            || (updateState_ == UPDATE_VERSION_STATE_DONE && updateError_ == UPDATE_VERSION_ERROR_NO_ERROR);
#endif
        if (!updateAvailable_.isEmpty() && !isInstallerStarted) {
            // The outcome of an earlier attempt must not be read as this one's.
            updateState_ = UPDATE_VERSION_STATE_INIT;
            updateError_ = UPDATE_VERSION_ERROR_NO_ERROR;
            updateProgress_ = 0;
#ifdef CLI_ONLY
            backend_->sendUpdateVersion(0);
#else
            emit update();
#endif
        }
    } else if (command->getStringId() == IPC::CliCommands::SetKeyLimitBehavior::getCommandStringId()) {
        IPC::CliCommands::SetKeyLimitBehavior *cmd = static_cast<IPC::CliCommands::SetKeyLimitBehavior *>(command);
        emit setKeyLimitBehavior(cmd->keyLimitDelete_);
    } else if (command->getStringId() == IPC::CliCommands::RotateIp::getCommandStringId()) {
        IPC::CliCommands::Acknowledge cmd;
        if (awaitingIpRotateResult_ || backend_->isIpRotateInFlight()) {
            cmd.code_ = 2;
            cmd.message_ = "IP rotate already in progress";
        } else if (!isBridgeApiAvailable_) {
            cmd.code_ = 1;
            cmd.message_ = "Bridge API not available";
        } else if (!backend_->rotateIp()) {
            cmd.code_ = 1;
            cmd.message_ = "Could not rotate IP.";
        } else {
            // Only one rotation can be in flight, so the next result is this one's.
            awaitingIpRotateResult_ = true;
            return;
        }
        sendCommand(cmd);
        return;
    } else if (command->getStringId() == IPC::CliCommands::FavIp::getCommandStringId()) {
        IPC::CliCommands::Acknowledge ack;
        if (!isBridgeApiAvailable_) {
            ack.code_ = 1;
            ack.message_ = "Bridge API not available";
        } else if (backend_->isAwaitingRotatedIp()) {
            ack.code_ = 2;
            ack.message_ = "IP rotate in progress";
        } else if (connectState_.connectState == CONNECT_STATE_CONNECTED && !isMyIpCurrent()) {
            // The address is from before this connect state: the previous server, or not looked up yet.
            qCWarning(LOG_BASIC) << "Could not pin IP" << myIp_;
            ack.code_ = 3;
            ack.message_ = "Current IP is not known yet";
        } else if (connectState_.connectState != CONNECT_STATE_CONNECTED ||
                   !backend_->locationsModelManager()->pinIp(backend_->currentLocation(),
                                                             backend_->getCurrentConnectingHostname(), myIp_)) {
            qCWarning(LOG_BASIC) << "Could not pin IP" << myIp_;
            ack.code_ = 1;
            ack.message_ = "Could not favourite IP";
        } else {
            qCInfo(LOG_BASIC) << "Pinning IP" << myIp_ << "for hostname" << backend_->getCurrentConnectingHostname();
        }
        sendCommand(ack);
        return;
    } else if (command->getStringId() == IPC::CliCommands::UnfavIp::getCommandStringId()) {
        IPC::CliCommands::UnfavIp *cmd = static_cast<IPC::CliCommands::UnfavIp *>(command);
        IPC::CliCommands::Acknowledge ack;
        if (!NetworkingValidation::isIp(cmd->ip_)) {
            ack.code_ = 2;
            ack.message_ = "Invalid IP address";
        } else if (!backend_->locationsModelManager()->unpinIp(cmd->ip_)) {
            qCDebug(LOG_BASIC) << "IP" << cmd->ip_ << "not found in favorites";
            ack.code_ = 1;
            ack.message_ = "Could not unfavourite IP";
        } else {
            qCInfo(LOG_BASIC) << "Unpinned IP" << cmd->ip_;
        }
        sendCommand(ack);
        return;
    } else if (command->getStringId() == IPC::CliCommands::ShowPorts::getCommandStringId()) {
        IPC::CliCommands::ShowPorts *cmd = static_cast<IPC::CliCommands::ShowPorts *>(command);
        types::Protocol protocol = types::Protocol::fromString(cmd->protocol_);
        IPC::CliCommands::PortsList response;
        response.ports_ = backend_->getPreferencesHelper()->getAvailablePortsForProtocol(protocol);
        reply(connection, response);
        return;
    } else if (command->getStringId() == IPC::CliCommands::ShowAmneziawg::getCommandStringId()) {
        IPC::CliCommands::AmneziawgPresetsList response;
        response.presets_ = backend_->getPreferencesHelper()->amneziawgPresets();
        reply(connection, response);
        return;
    }

    IPC::CliCommands::Acknowledge cmd;
    sendCommand(cmd);
}

void LocalIPCServer::onConnectionStateCallback(int state, IPC::Connection *connection)
{
    if (state == IPC::CONNECTION_DISCONNECTED || state == IPC::CONNECTION_ERROR) {
        if (state == IPC::CONNECTION_ERROR) {
            qCWarning(LOG_BASIC) << "CLI disconnected from server with error";
        }
        // A repeat stateChanged for a connection already torn down must not close() it again.
        if (!connections_.removeOne(connection)) {
            return;
        }
        connection->close();
        connection->deleteLater();

        // A rotation or log upload already asked for still goes ahead, but its answer belongs to nobody now, and must not
        // block or reach the next CLI.
        awaitingIpRotateResult_ = false;
        awaitingDebugLogResult_ = false;
    }
}

void LocalIPCServer::onBackendLoginFinished()
{
    loginState_ = LOGIN_STATE_LOGGED_IN;
    lastLoginError_ = wsnet::LoginResult::kSuccess;
}

void LocalIPCServer::onBackendCaptchaRequired(bool isAsciiCaptcha, const QString &asciiArt, const QString &background, const QString &slider, int top)
{
    isCaptchaRequired_ = true;
    asciiArt_ = asciiArt;
}

void LocalIPCServer::onBackendLoginError(wsnet::LoginResult code, const QString &msg)
{
    lastLoginError_ = code;
    lastLoginErrorMessage_ = msg;
}

void LocalIPCServer::onBackendLogoutFinished()
{
    loginState_ = LOGIN_STATE_LOGGED_OUT;
    lastLoginError_ = wsnet::LoginResult::kSuccess;
}

void LocalIPCServer::sendCommand(const IPC::Command &command)
{
    // A write failure inside sendCommand() synchronously emits stateChanged, whose handler removes the
    // connection from connections_; iterate a copy and skip connections torn down earlier in the loop.
    const auto connections = connections_;
    for (IPC::Connection *connection : connections) {
        if (connections_.contains(connection)) {
            connection->sendCommand(command);
        }
    }
}

void LocalIPCServer::reply(IPC::Connection *connection, const IPC::Command &command)
{
    if (connections_.contains(connection)) {
        connection->sendCommand(command);
    }
}

void LocalIPCServer::sendState(IPC::Connection *connection)
{
    IPC::CliCommands::State cmd;
    cmd.language_ = backend_->getPreferences()->language();
    cmd.connectivity_ = connectivity_;
    cmd.loginState_ = backend_->currentLoginState();
    cmd.loginError_ = lastLoginError_;
    cmd.loginErrorMessage_ = lastLoginErrorMessage_;
    cmd.connectState_ = connectState_;
    cmd.isCaptchaRequired_ = isCaptchaRequired_;
    cmd.asciiArt_ = asciiArt_;
    cmd.connectId_ = connectId_;
    cmd.tunnelTestState_ = tunnelTestState_;
    cmd.protocol_ = protocol_;
    cmd.port_ = port_;
    cmd.location_ = backend_->currentLocation();
    cmd.isFirewallOn_ = backend_->isFirewallEnabled();
    cmd.isFirewallAlwaysOn_ = backend_->isFirewallAlwaysOn();
    cmd.updateState_ = updateState_;
    cmd.updateError_ = updateError_;
    cmd.updateProgress_ = updateProgress_;
    cmd.updatePath_ = updatePath_;
    cmd.updateAvailable_ = updateAvailable_;
    cmd.trafficUsed_ = backend_->getAccountInfo()->trafficUsed();
    cmd.trafficMax_ = backend_->getAccountInfo()->plan();
    // An IP from before the last connect state change belongs to the previous state, so leave it out rather than
    // show the VPN IP as public, the public IP as VPN, or the previous server's IP.
    cmd.myIp_ = isMyIpCurrent() ? myIp_ : QString();
    cmd.connectRequestCount_ = backend_->connectRequestCount();
    reply(connection, cmd);
}

void LocalIPCServer::onBackendCheckUpdateChanged(const api_responses::CheckUpdate &info)
{
    if (info.isAvailable()) {
        updateAvailable_ = info.version() + "." + QString::number(info.latestBuild());
    } else {
        updateAvailable_ = "";
    }
}

void LocalIPCServer::onBackendConnectStateChanged(const types::ConnectState &state)
{
    if (state.connectState != connectState_.connectState) {
        isMyIpStale_ = true;
    }
    connectState_ = state;
    tunnelTestState_ = TUNNEL_TEST_STATE_UNKNOWN;

    if (connectState_.connectState == CONNECT_STATE_DISCONNECTED) {
        // Normally these look like a regular disconnect; override with a special reason
        if (disconnectedByKeyLimit_) {
            connectState_.disconnectReason = DISCONNECTED_BY_KEY_LIMIT;
            disconnectedByKeyLimit_ = false;
        } else if (invalidLocation_) {
            connectState_.connectError = ConnectError::kLocationUnavailable;
            invalidLocation_ = false;
        }
    }
}

void LocalIPCServer::onBackendProtocolPortChanged(const types::Protocol &protocol, uint port)
{
    protocol_ = protocol;
    port_ = port;
}

void LocalIPCServer::onBackendInternetConnectivityChanged(bool connectivity)
{
    connectivity_ = connectivity;
}

void LocalIPCServer::onBackendMyIpChanged(const QString &ip, bool isFromDisconnectedState)
{
    myIp_ = ip;
    isMyIpFromDisconnectedState_ = isFromDisconnectedState;
    isMyIpStale_ = false;
}

bool LocalIPCServer::isMyIpCurrent() const
{
    if (isMyIpStale_) {
        return false;
    }
    return (connectState_.connectState == CONNECT_STATE_DISCONNECTED && isMyIpFromDisconnectedState_) ||
           (connectState_.connectState == CONNECT_STATE_CONNECTED && !isMyIpFromDisconnectedState_);
}

void LocalIPCServer::onBackendTestTunnelResult(bool success)
{
    tunnelTestState_ = success ? TUNNEL_TEST_STATE_SUCCESS : TUNNEL_TEST_STATE_FAILURE;
}

void LocalIPCServer::onBackendUpdateVersionChanged(uint progressPercent, UPDATE_VERSION_STATE state, UPDATE_VERSION_ERROR error)
{
    updateState_ = state;

    if (state == UPDATE_VERSION_STATE_DONE) {
        updateError_ = error;
    } else if (state == UPDATE_VERSION_STATE_DOWNLOADING) {
        updateProgress_ = progressPercent;
    } else if (state == UPDATE_VERSION_STATE_RUNNING) {
        updateProgress_ = 100;
    }
}

void LocalIPCServer::onBackendUpdateDownloaded(const QString &path)
{
    updatePath_ = path;
}

void LocalIPCServer::setDisconnectedByKeyLimit()
{
    disconnectedByKeyLimit_ = true;
}

void LocalIPCServer::onBackendConnectionIdChanged(const QString &connId)
{
    connectId_ = connId;
}

void LocalIPCServer::onBackendBridgeApiAvailabilityChanged(bool isAvailable)
{
    isBridgeApiAvailable_ = isAvailable;
}

void LocalIPCServer::onLocationsModelManagerDeviceNameChanged(const QString &deviceName)
{
    deviceName_ = deviceName;
}

void LocalIPCServer::onBackendIpRotateResult(bool success)
{
    if (!awaitingIpRotateResult_) {
        return;
    }

    awaitingIpRotateResult_ = false;

    IPC::CliCommands::Acknowledge cmd;
    if (success) {
        cmd.code_ = 0;
        cmd.message_ = "IP rotated.";
    } else {
        cmd.code_ = 1;
        cmd.message_ = "Could not rotate IP.";
    }
    sendCommand(cmd);
}

void LocalIPCServer::onBackendDebugLogResult(bool success)
{
    if (!awaitingDebugLogResult_) {
        return;
    }

    awaitingDebugLogResult_ = false;

    IPC::CliCommands::Acknowledge cmd;
    cmd.code_ = success ? 0 : 1;
    sendCommand(cmd);
}

IPC::CliCommands::Acknowledge LocalIPCServer::hydraProfileAnswer(const IPC::CliCommands::SetHydraProfile &cmd) const
{
    IPC::CliCommands::Acknowledge ack;
    std::optional<HydraProfile> profile;
    QString error;
    if (!cmd.isValid_) {
        error = tr("Malformed hydra profile command.");
    } else if (cmd.remove_) {
        if (!HydraProfile::remove()) {
            error = tr("Could not remove the hydra profile.");
        } else {
            qCInfo(LOG_BASIC) << "Hydra profile removed";
            ack.message_ = tr("Hydra profile removed.");
            return ack;
        }
    } else if (!cmd.json_.isEmpty()) {
        profile = HydraProfile::save(cmd.json_, error);
        if (profile) {
            qCInfo(LOG_BASIC) << "Hydra profile imported:" << profile->fronts().size() << "front(s)";
        }
    } else {
        profile = HydraProfile::load(&error);
        if (!profile && error.isEmpty()) {
            ack.message_ = tr("No hydra profile imported.");
            return ack;
        }
        if (!profile) {
            // A file the app can no longer read, such as a version 1 profile: say why it does nothing.
            error = tr("The imported hydra profile is not in use. %1").arg(error);
        }
    }

    if (!profile) {
        ack.code_ = 1;
        ack.message_ = error;
        return ack;
    }
    QStringList lines;
    lines << tr("Hydra profile %1:").arg(profile->id().isEmpty() ? tr("(unnamed)") : profile->id());
    const gui_locations::LocationsModelManager *locations = backend_->locationsModelManager();
    const QStringList fronts = profile->summary([locations](int datacenterId) {
        return locations->cityNameForDatacenter(datacenterId);
    });
    for (const QString &front : fronts) {
        lines << "  " + front;
    }
    if (profile->all()) {
        lines << tr("It maps every location: all of them connect through your hydra server.");
    } else {
        lines << tr("Connect to one of these locations to go through your hydra server; the others connect directly.");
    }
    ack.message_ = lines.join("\n");
    return ack;
}
