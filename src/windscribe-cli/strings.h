#include <QList>
#include <QPair>
#include <QString>

#include "ipc/clicommands.h"
#include "ipc/connection.h"
#include "types/locationid.h"

QString connectivityString(bool connectivity);
QString loginStateString(LOGIN_STATE state, wsnet::LoginResult loginError, const QString &loginErrorMessage);
QString connectStateString(types::ConnectState state, LocationID location, TUNNEL_TEST_STATE tunnelTest);
QString ipString(const QString &ip, bool isConnected);
QString protocolString(types::Protocol protocol, uint port);
QString firewallStateString(bool isFirewallOn, bool isFirewallAlwaysOn);
QString dataString(qint64 data);
QString updateErrorString(UPDATE_VERSION_ERROR error);
QString deviceNameString(const QString &deviceName);
QString locationsString(const QList<IPC::CliCommands::IpcLocation> &locations);
QString appRunningInAnotherSessionString();
QString notLoggedInString();
QString noConnectivityString();
// Lines of "Label: value", with the values lined up.
QString statusString(const QList<QPair<QString, QString>> &items);
