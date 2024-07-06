#include "strings.h"

QString connectivityString(bool connectivity)
{
    return QObject::tr("Internet connectivity: %1").arg(connectivity ? QObject::tr("available") : QObject::tr("unavailable"));
}

QString loginStateString(LOGIN_STATE state, LOGIN_RET loginError, const QString &loginErrorMessage)
{
    QString msg = QObject::tr("Login state: %1");

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
        case LOGIN_RET_NO_API_CONNECTIVITY:
            return msg.arg(error.arg(QObject::tr("No internet connectivity")));
        case LOGIN_RET_NO_CONNECTIVITY:
            return msg.arg(error.arg(QObject::tr("No API connectivity")));
        case LOGIN_RET_BAD_USERNAME:
        case LOGIN_RET_MISSING_CODE2FA:
            return msg.arg(error.arg(QObject::tr("Incorrect username, password, or 2FA code")));
        case LOGIN_RET_ACCOUNT_DISABLED:
            return msg.arg(error.arg(loginErrorMessage));
        case LOGIN_RET_SSL_ERROR:
            return msg.arg(error.arg(QObject::tr("SSL error")));
        case LOGIN_RET_SESSION_INVALID:
            return msg.arg(error.arg(QObject::tr("Session expired")));
        case LOGIN_RET_RATE_LIMITED:
            return msg.arg(error.arg(QObject::tr("Rate limited")));
        case LOGIN_RET_INCORRECT_JSON:
        case LOGIN_RET_SUCCESS:
        default:
            return msg.arg(QObject::tr("Unknown error"));
        }
    }
}

QString connectStateString(types::ConnectState state, LocationID location, TUNNEL_TEST_STATE tunnelTest)
{
    QString msg = QObject::tr("Connect state: %1");
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

QString updateString(UPDATE_VERSION_ERROR error, uint progress, const QString &updateAvailable)
{
    if (error == UPDATE_VERSION_ERROR_DL_FAIL) {
        return QObject::tr("Update error: download failed. Please try again or try a different network.");
    } else if (error != UPDATE_VERSION_ERROR_NO_ERROR) {
        return QObject::tr("Update error: %1").arg(error);
    } else if (progress != 0) {
        return QObject::tr("Update downloading: %1%%").arg(progress);
    } else if (!updateAvailable.isEmpty()) {
        return QObject::tr("Update available: %1").arg(updateAvailable);
    }
    return QObject::tr("No update available");
}

