#include <QString>

#include "ipc/clicommands.h"
#include "ipc/connection.h"
#include "types/locationid.h"

QString connectivityString(bool connectivity);
QString loginStateString(LOGIN_STATE state, LOGIN_RET loginError, const QString &loginErrorMessage);
QString connectStateString(types::ConnectState state, LocationID location, TUNNEL_TEST_STATE tunnelTest);
QString protocolString(types::Protocol protocol, uint port);
QString firewallStateString(bool isFirewallOn, bool isFirewallAlwaysOn);
QString dataString(const QString &language, qint64 data);
QString updateString(UPDATE_VERSION_ERROR error, uint progress, const QString &updateAvailable);

