#pragma once

#include <QString>

enum CliCommand {
    CLI_COMMAND_NONE,
    CLI_COMMAND_HELP,
    CLI_COMMAND_INVALID,
    CLI_COMMAND_VERSION,
    CLI_COMMAND_CONNECT,
    CLI_COMMAND_CONNECT_BEST,
    CLI_COMMAND_CONNECT_LOCATION,
    CLI_COMMAND_CONNECT_STATIC,
    CLI_COMMAND_DISCONNECT,
    CLI_COMMAND_FIREWALL_ON,
    CLI_COMMAND_FIREWALL_OFF,
    CLI_COMMAND_IP_ROTATE,
    CLI_COMMAND_IP_FAV,
    CLI_COMMAND_IP_UNFAV,
    CLI_COMMAND_LOCATIONS,
    CLI_COMMAND_LOCATIONS_FAV,
    CLI_COMMAND_LOCATIONS_STATIC,
    CLI_COMMAND_LOGIN,
    CLI_COMMAND_LOGOUT,
    CLI_COMMAND_RELOAD_CONFIG,
    CLI_COMMAND_SEND_LOGS,
    CLI_COMMAND_SET_IGNORE_SSL_ERRORS,
    CLI_COMMAND_SET_KEYLIMIT_BEHAVIOR,
    CLI_COMMAND_STATUS,
    CLI_COMMAND_UPDATE,
    CLI_COMMAND_AMNEZIAWG,
    CLI_COMMAND_PORTS,
    CLI_COMMAND_HYDRA_SHOW,
    CLI_COMMAND_HYDRA_IMPORT,
    CLI_COMMAND_HYDRA_REMOVE,
};

class CliArguments
{
public:
    explicit CliArguments();
    void processArguments();

    CliCommand cliCommand() const;
    // The command to show help for (CLI_COMMAND_HELP), or whose arguments are invalid (CLI_COMMAND_INVALID).
    const QString &helpTopic() const;
    // Why the arguments are invalid (CLI_COMMAND_INVALID).
    const QString &error() const;
    const QString &location() const;
    const QString &username() const;
    const QString &password() const;
    const QString &protocol() const;
    const uint &port() const;
    const QString &ipAddress() const;
    const QString &hydraFile() const;
    bool keepFirewallOn() const;
    bool keyLimitDelete() const;
    bool ignoreSslErrors() const;
    bool nonBlocking() const;

private:
    CliCommand cliCommand_ = CLI_COMMAND_NONE;
    QString helpTopic_;
    QString error_;
    QString locationStr_ = "";
    QString username_;
    QString password_;
    QString protocol_ = "";
    uint port_ = 0;
    QString ipAddress_ = "";
    QString hydraFile_;
    bool keepFirewallOn_ = false;
    bool keyLimitDelete_ = false;
    bool ignoreSslErrors_ = false;
    bool nonBlocking_ = false;

    void setError(const QString &command, const QString &error);
    bool parseProtocol(const QString &command, const QString &arg);
    void parseConnect(const QStringList &args);
    void parseDisconnect(const QStringList &args);
    void parseFirewall(const QStringList &args);
    void parseIp(const QStringList &args);
    void parseLocations(const QStringList &args);
    void parseLogin(const QStringList &args);
    void parsePorts(const QStringList &args);
    void parseHydra(const QStringList &args);
    void parseLogout(const QStringList &args);
    void parsePreferences(const QStringList &args);
    void parseLogs(const QStringList &args);
    void parseKeyLimit(const QStringList &args);
    void parseIgnoreSslErrors(const QStringList &args);
};
