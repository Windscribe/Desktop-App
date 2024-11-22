#pragma once

#include <QString>

enum CliCommand {
    CLI_COMMAND_NONE,
    CLI_COMMAND_HELP,
    CLI_COMMAND_CONNECT,
    CLI_COMMAND_CONNECT_BEST,
    CLI_COMMAND_CONNECT_LOCATION,
    CLI_COMMAND_CONNECT_STATIC,
    CLI_COMMAND_DISCONNECT,
    CLI_COMMAND_FIREWALL_ON,
    CLI_COMMAND_FIREWALL_OFF,
    CLI_COMMAND_LOCATIONS,
    CLI_COMMAND_LOCATIONS_STATIC,
    CLI_COMMAND_LOGIN,
    CLI_COMMAND_LOGOUT,
    CLI_COMMAND_RELOAD_CONFIG,
    CLI_COMMAND_SEND_LOGS,
    CLI_COMMAND_SET_KEYLIMIT_BEHAVIOR,
    CLI_COMMAND_STATUS,
    CLI_COMMAND_UPDATE,
};

class CliArguments
{
public:
    explicit CliArguments();
    void processArguments();

    CliCommand cliCommand() const;
    const QString &location() const;
    const QString &username() const;
    const QString &password() const;
    const QString &code2fa() const;
    const QString &protocol() const;
    bool keepFirewallOn() const;
    bool keyLimitDelete() const;
    bool nonBlocking() const;
    bool need2FA() const;

    void setUsername(const QString &username);
    void setPassword(const QString &password);
    void set2FACode(const QString &code);

private:
    CliCommand cliCommand_ = CLI_COMMAND_NONE;
    QString locationStr_ = "";
    QString username_;
    QString password_;
    QString code2fa_;
    QString protocol_ = "";
    bool keepFirewallOn_ = false;
    bool keyLimitDelete_ = false;
    bool nonBlocking_ = false;
    bool need2FA_ = false;

    void parseConnect(const QStringList &args);
    void parseDisconnect(const QStringList &args);
    void parseFirewall(const QStringList &args);
    void parseLocations(const QStringList &args);
    void parseLogin(const QStringList &args);
    void parseLogout(const QStringList &args);
    void parsePreferences(const QStringList &args);
    void parseLogs(const QStringList &args);
    void parseKeyLimit(const QStringList &args);
};
