#include "strings.h"

using namespace wsnet;

QString connectivityString(bool connectivity)
{
    return connectivity ? QString("Available") : QString("Unavailable");
}

QString loginStateString(LOGIN_STATE state, wsnet::LoginResult loginError, const QString &loginErrorMessage)
{
    switch (state) {
    case LOGIN_STATE_LOGGED_OUT:
        return QString("Logged out");
    case LOGIN_STATE_LOGGING_IN:
        return QString("Logging in");
    case LOGIN_STATE_LOGGED_IN:
        return QString("Logged in");
    case LOGIN_STATE_LOGIN_ERROR:
    default:
        QString error = QString("Error: %1");

        switch(loginError) {
        case LoginResult::kNoConnectivity:
            return error.arg(QString("No internet connectivity"));
        case LoginResult::kNoApiConnectivity:
            return error.arg(QString("No API connectivity"));
        case LoginResult::kBadUsername:
            return error.arg(QString("Incorrect username, password, or 2FA code"));
        case LoginResult::kMissingCode2fa:
            return error.arg(QString("Need 2FA code"));
        case LoginResult::kAccountDisabled:
        case LoginResult::kInvalidSecurityToken:
            return error.arg(loginErrorMessage);
        case LoginResult::kSslError:
#ifdef CLI_ONLY
            return QString("Could not log in. Please try again. If the problem continues, you can ignore SSL errors for this session, but we strongly recommend against it: doing so disables TLS certificate validation and could allow a third party to intercept your traffic. To proceed anyway, run \"windscribe-cli ignoresslerrors on\".");
#else
            return error.arg(QString("SSL error"));
#endif
        case LoginResult::kSessionInvalid:
            return error.arg(QString("Session expired"));
        case LoginResult::kRateLimited:
            return error.arg(QString("Rate limited"));
        case LoginResult::kBadCode2fa:
            return error.arg(QString("Incorrect 2FA code"));
        case LoginResult::kIncorrectJson:
        case LoginResult::kSuccess:
        default:
            return QString("Unknown error");
        }
    }
}

QString connectStateString(types::ConnectState state, LocationID location, TUNNEL_TEST_STATE tunnelTest)
{
    QString connectState;

    switch (state.connectState) {
    case CONNECT_STATE_CONNECTED:
        if (location.isValid()) {
            connectState = QString("Connected to %1").arg(location.city());
        } else {
            connectState = QString("Connected");
        }
        if (tunnelTest == TUNNEL_TEST_STATE_UNKNOWN) {
            connectState += " " + QString("(checking the connection)");
        } else if (tunnelTest == TUNNEL_TEST_STATE_FAILURE) {
            connectState += " " + QString("(network interference detected)");
        }
        return connectState;
    case CONNECT_STATE_CONNECTING:
        if (location.isValid()) {
            return QString("Connecting to %1").arg(location.city());
        } else {
            return QString("Connecting");
        }
    case CONNECT_STATE_DISCONNECTING:
        return QString("Disconnecting");
    case CONNECT_STATE_DISCONNECTED:
        if (state.disconnectReason == DISCONNECTED_BY_KEY_LIMIT) {
            return QString("Disconnected due to reaching WireGuard key limit. Use \"windscribe-cli keylimit delete\" if you want to delete the oldest key instead, and try again.");
        }

        switch(state.connectError) {
        case ConnectError::kNoError:
            return QString("Disconnected");
        case ConnectError::kLocationUnavailable:
            return QString("Error: Location does not exist or is disabled");
        case ConnectError::kAccountBlocked:
            return QString("Error: You are out of data, or your account has been disabled. Upgrade to Pro to continue using Windscribe");
        case ConnectError::kDnsServiceStartFailure:
            return QString("Error: Unable to start custom DNS service");
        case ConnectError::kHostsFileNotWritable:
            return QString("Error: Could not update the hosts file required by IKEv2");
        case ConnectError::kAdapterSetupFailure:
            return QString("Error: WireGuard adapter setup failed");
        case ConnectError::kAdapterNotInstalled:
            return QString("Error: VPN adapter setup failed");
        case ConnectError::kConfigFetchFailure:
            return QString("Error: Could not retrieve WireGuard configuration");
        case ConnectError::kLocalConfigGenerationFailure:
            return QString("Error: Could not generate connection configuration");
        case ConnectError::kLocalDnsServerNotAvailable:
            return QString("Error: Local DNS server is not available. Connected DNS has been set back to Auto");
        case ConnectError::kHydraUnreachable:
            return QString("Error: Can't reach your hydra server at %1. Check that it's running (hydra doctor), "
                           "or remove the hydra profile (windscribe-cli hydra remove) to connect without it")
                .arg(state.connectErrorDetail);
        case ConnectError::kHydraNeedsWireGuard:
            return QString("Error: This location is set to connect through your hydra server, which works with "
                           "WireGuard only. Connect with WireGuard, or remove the hydra profile (windscribe-cli hydra remove)");
        case ConnectError::kCustomConfigInvalid:
            if (!state.connectErrorDetail.isEmpty()) {
                return QString("Error: The custom configuration could not be loaded") + QString(". ") + state.connectErrorDetail;
            }
            return QString("Error: The custom configuration could not be loaded");
        case ConnectError::kAuthFailure:
            return QString("Error: The server did not accept the connection credentials");
        case ConnectError::kPrivKeyPasswordFailure:
            return QString("Error: The private key password for the custom configuration is incorrect");
        case ConnectError::kLocalProcessLaunchFailure:
            if (!state.connectErrorDetail.isEmpty()) {
                return QString("Error: A required VPN process could not be started") + QString(". ") + state.connectErrorDetail;
            }
            return QString("Error: A required VPN process could not be started");
        case ConnectError::kLocalProcessNotResponding:
            return QString("Error: A required VPN process is not responding");
        case ConnectError::kTransientTunnelFailure:
        case ConnectError::kTunnelEstablishmentFailure:
            return QString("Error: The VPN tunnel could not be established. Please try a different protocol");
        case ConnectError::kVpnServiceSetupFailure:
            return QString("Error: The system VPN service could not be set up");
        case ConnectError::kBlockedByOsPolicy:
            return QString("Error: IKEv2 connectivity is not available in macOS Lockdown Mode. "
                           "Please disable Lockdown Mode in System Settings or change your connection settings");
        case ConnectError::kOSServiceUnavailable:
            return QString("Error: The Remote Access Connection Manager (RasMan) service is not running and could not be started");
        default:
            if (!state.connectErrorDetail.isEmpty()) {
                return QString("Error: %1").arg(static_cast<int>(state.connectError)) + QString(". ") + state.connectErrorDetail;
            }
            return QString("Error: %1").arg(static_cast<int>(state.connectError));
        }
    default:
        return QString("Unknown state");
    }
}

QString ipString(const QString &ip, bool isConnected)
{
    if (ip.isEmpty()) {
        return QString();
    }
    return isConnected ? QString("%1 (VPN)").arg(ip) : QString("%1 (public)").arg(ip);
}

QString protocolString(types::Protocol protocol, uint port)
{
    return QString("%1:%2").arg(protocol.toLongString()).arg(port);
}

QString firewallStateString(bool isFirewallOn, bool isFirewallAlwaysOn)
{
    if (isFirewallAlwaysOn) {
        return QString("Always On");
    }
    else if (isFirewallOn) {
        return QString("On");
    }
    else {
        return QString("Off");
    }
}

QString dataString(qint64 data)
{
    return QLocale::c().formattedDataSize(data, 2, QLocale::DataSizeTraditionalFormat);
}

QString updateErrorString(UPDATE_VERSION_ERROR error)
{
    if (error == UPDATE_VERSION_ERROR_DL_FAIL) {
        return QString("Update error: download failed. Please try again or try a different network.");
    } else if (error == UPDATE_VERSION_ERROR_CANCELLED || error == UPDATE_VERSION_ERROR_ELEVATION_DECLINED) {
        return QString("Update cancelled.");
    } else if (error == UPDATE_VERSION_ERROR_SIGN_FAIL) {
        return QString("Update error: the installer signature could not be verified.");
    } else if (error == UPDATE_VERSION_ERROR_START_INSTALLER_FAIL) {
        return QString("Update error: the installer could not be started.");
    } else if (error == UPDATE_VERSION_ERROR_OTHER_FAIL) {
        return QString("Update error: the installer failed.");
    } else if (error != UPDATE_VERSION_ERROR_NO_ERROR) {
        return QString("Update error: %1").arg(error);
    } else {
        return "";
    }
}

QString deviceNameString(const QString &deviceName)
{
    return QString("Device name: %1").arg(deviceName);
}

QString locationsString(const QList<IPC::CliCommands::IpcLocation> &locations)
{
    QString msg;

    for(const IPC::CliCommands::IpcLocation &location : locations) {
        QString locationStr;

        if (location.city.isEmpty()) { // Best Location
            locationStr = QString("%1 - %2").arg(location.country, location.nickname);
        } else if (location.country.isEmpty()) { // Static IP
            locationStr = QString("%1 - %2").arg(location.city, location.nickname);
        } else {
            locationStr = QString("%1 - %2 - %3").arg(location.country, location.city, location.nickname);
        }

        // Add pinned IP if present
        if (!location.pinnedIp.isEmpty()) {
            locationStr += QString(" [%1]").arg(location.pinnedIp);
        }

        if (location.flags & (int)IPC::CliCommands::LocationFlags::kDisabled) {
            if (location.flags & (int)IPC::CliCommands::LocationFlags::kPremium) {
                locationStr += QString(" (Pro)");
            } else {
                locationStr += QString(" (Disabled)");
            }
        }
        if (location.flags & (int)IPC::CliCommands::LocationFlags::k10Gbps) {
            locationStr += QString(" (10 Gbps)");
        }

        msg += locationStr + "\n";
    }

    return msg.trimmed();
}

QString appRunningInAnotherSessionString()
{
    return QString("Windscribe is already running in another user session on this computer. "
                       "Quit Windscribe in that session, then try again.");
}

QString notLoggedInString()
{
    return QString("Not logged in. Run \"windscribe-cli login\" first.");
}

QString noConnectivityString()
{
    return QString("Internet connectivity is not available. Try again later.");
}

QString statusString(const QList<QPair<QString, QString>> &items)
{
    qsizetype width = 0;
    for (const auto &item : items) {
        width = qMax(width, item.first.length());
    }

    QStringList lines;
    for (const auto &item : items) {
        // Line the values up by the longest label.
        lines << (item.first + ":").leftJustified(width + 3) + item.second;
    }
    return lines.join("\n");
}
