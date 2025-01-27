#include <QString>

#include "ipc/clicommands.h"
#include "ipc/connection.h"
#include "types/locationid.h"

QString connectivityString(bool connectivity);
QString loginStateString(LOGIN_STATE state, wsnet::LoginResult loginError, const QString &loginErrorMessage, bool showPrefix = true);
QString connectStateString(types::ConnectState state, LocationID location, TUNNEL_TEST_STATE tunnelTest, bool showPrefix = true);
QString protocolString(types::Protocol protocol, uint port);
QString firewallStateString(bool isFirewallOn, bool isFirewallAlwaysOn);
QString dataString(const QString &language, qint64 data);
QString updateString(const QString &updateAvailable);
QString updateErrorString(UPDATE_VERSION_ERROR error);
QString deviceNameString(const QString &deviceName);
