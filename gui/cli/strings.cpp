#include "strings.h"

using namespace wsnet;

QString connectivityString(bool connectivity)
{
    return QObject::tr("Internet connectivity: %1").arg(connectivity ? QObject::tr("available") : QObject::tr("unavailable"));
}

QString loginStateString(LOGIN_STATE state, wsnet::LoginResult loginError, const QString &loginErrorMessage, bool showPrefix)
{
    QString msg = showPrefix ? QObject::tr("Login state: %1") : "%1";

    switch (state) {
    case LOGIN_STATE_LOGGED_OUT:
        return msg.arg(QObject::tr("Logged out"));
    case LOGIN_STATE_LOGGING_IN:
        return msg.arg(QObject::tr("Logging in"));
    case LOGIN_STATE_LOGGED_IN:
        return msg.arg(QObject::tr("Logged in"));
    case LOGIN_STATE_LOGIN_ERROR:
    default:
        QString error = QObject::tr("Error: %1");

        switch(loginError) {
        case LoginResult::kNoConnectivity:
            return msg.arg(error.arg(QObject::tr("No internet connectivity")));
        case LoginResult::kNoApiConnectivity:
            return msg.arg(error.arg(QObject::tr("No API connectivity")));
        case LoginResult::kBadUsername:
            return msg.arg(error.arg(QObject::tr("Incorrect username, password, or 2FA code")));
        case LoginResult::kMissingCode2fa:
            return msg.arg(error.arg(QObject::tr("Need 2FA code")));
        case LoginResult::kAccountDisabled:
        case LoginResult::kInvalidSecurityToken:
            return msg.arg(error.arg(loginErrorMessage));
        case LoginResult::kSslError:
            return msg.arg(error.arg(QObject::tr("SSL error")));
        case LoginResult::kSessionInvalid:
            return msg.arg(error.arg(QObject::tr("Session expired")));
        case LoginResult::kRateLimited:
            return msg.arg(error.arg(QObject::tr("Rate limited")));
        case LoginResult::kBadCode2fa:
            return msg.arg(error.arg(QObject::tr("Incorrect 2FA code")));
        case LoginResult::kIncorrectJson:
        case LoginResult::kSuccess:
        default:
            return msg.arg(QObject::tr("Unknown error"));
        }
    }
}

QString connectStateString(types::ConnectState state, LocationID location, TUNNEL_TEST_STATE tunnelTest, bool showPrefix)
{
    QString msg = showPrefix ? QObject::tr("Connect state: %1") : "%1";
    QString connectState;

    switch (state.connectState) {
    case CONNECT_STATE_CONNECTED:
        if (location.isValid()) {
            connectState = msg.arg(QObject::tr("Connected: %1").arg(location.city()));
        } else {
            connectState = msg.arg(QObject::tr("Connected"));
        }
        if (tunnelTest == TUNNEL_TEST_STATE_UNKNOWN) {
            connectState = "*" + connectState;
        } else if (tunnelTest == TUNNEL_TEST_STATE_FAILURE) {
            connectState += " " + QObject::tr("[Network interference]");
        }
        return connectState;
    case CONNECT_STATE_CONNECTING:
        if (location.isValid()) {
            return msg.arg(QObject::tr("Connecting: %1").arg(location.city()));
        } else {
            return msg.arg(QObject::tr("Connecting"));
        }
    case CONNECT_STATE_DISCONNECTING:
        return msg.arg(QObject::tr("Disconnecting"));
    case CONNECT_STATE_DISCONNECTED:
        if (state.disconnectReason == DISCONNECTED_BY_KEY_LIMIT) {
            return msg.arg(QObject::tr("Disconnected due to reaching WireGuard key limit.  Use \"windscribe-cli keylimit delete\" if you want to delete the oldest key instead, and try again."));
        }

        switch(state.connectError) {
        case NO_CONNECT_ERROR:
            return msg.arg(QObject::tr("Disconnected"));
        case LOCATION_NOT_EXIST:
        case LOCATION_NO_ACTIVE_NODES:
            return msg.arg(QObject::tr("Error: Location does not exist or is disabled"));
        case CONNECTION_BLOCKED:
            return msg.arg(QObject::tr("Error: You are out of data, or your account has been disabled. Upgrade to Pro to continue using Windscribe"));
        case CTRLD_START_FAILED:
            return msg.arg(QObject::tr("Error: Unable to start custom DNS service"));
        case WIREGUARD_ADAPTER_SETUP_FAILED:
            return msg.arg(QObject::tr("Error: WireGuard adapter setup failed"));
        case WIREGUARD_COULD_NOT_RETRIEVE_CONFIG:
            return msg.arg(QObject::tr("Error: Could not retrieve WireGuard configuration"));
        default:
            return msg.arg(QObject::tr("Error: %1").arg(state.connectError));
        }
    default:
        return msg.arg(QObject::tr("Unknown state"));
    }
}

QString protocolString(types::Protocol protocol, uint port)
{
    return QObject::tr("Protocol: %1:%2").arg(protocol.toLongString()).arg(port);
}

QString firewallStateString(bool isFirewallOn, bool isFirewallAlwaysOn)
{
    QString msg = QObject::tr("Firewall state: %1");

    if (isFirewallAlwaysOn) {
        return msg.arg(QObject::tr("Always On"));
    }
    else if (isFirewallOn) {
        return msg.arg(QObject::tr("On"));
    }
    else {
        return msg.arg(QObject::tr("Off"));
    }
}

QString dataString(const QString &language, qint64 data)
{
    QLocale locale(language);
    return locale.formattedDataSize(data, 2, QLocale::DataSizeTraditionalFormat);
}

QString updateString(const QString &updateAvailable)
{
    if (!updateAvailable.isEmpty()) {
        return QObject::tr("Update available: %1").arg(updateAvailable);
    }
    return QObject::tr("No update available");
}

QString updateErrorString(UPDATE_VERSION_ERROR error)
{
    if (error == UPDATE_VERSION_ERROR_DL_FAIL) {
        return QObject::tr("Update error: download failed. Please try again or try a different network.");
    } else if (error != UPDATE_VERSION_ERROR_NO_ERROR) {
        return QObject::tr("Update error: %1").arg(error);
    } else {
        return "";
    }
}

QString deviceNameString(const QString &deviceName)
{
    return QObject::tr("(Device name: %1)").arg(deviceName);
}
