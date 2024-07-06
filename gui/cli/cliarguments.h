#pragma once

#include <QString>

enum CliCommand {
    CLI_COMMAND_NONE,
    CLI_COMMAND_HELP,
    CLI_COMMAND_CONNECT,
    CLI_COMMAND_CONNECT_BEST,
    CLI_COMMAND_CONNECT_LOCATION,
    CLI_COMMAND_DISCONNECT,
    CLI_COMMAND_FIREWALL_ON,
    CLI_COMMAND_FIREWALL_OFF,
    CLI_COMMAND_LOCATIONS,
    CLI_COMMAND_LOGIN,
    CLI_COMMAND_LOGOUT,
    CLI_COMMAND_RELOAD_CONFIG,
    CLI_COMMAND_SEND_LOGS,
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

private:
    CliCommand cliCommand_ = CLI_COMMAND_NONE;
    QString locationStr_ = "";
    QString username_;
    QString password_;
    QString code2fa_;
    QString protocol_ = "";
    bool keepFirewallOn_ = false;
};
