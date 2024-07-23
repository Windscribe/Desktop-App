#include <QCoreApplication>
//#include <iostream>

#include "cliarguments.h"

CliArguments::CliArguments()
{
}

static bool isProtocol(const QString &str)
{
    if (str == "wireguard" || str == "ikev2" || str == "udp" || str == "tcp" || str == "stealth" || str == "wstunnel") {
        return true;
    }
    return false;
}

void CliArguments::parseConnect(const QStringList &args)
{
    if (args.length() <= 2) {
        cliCommand_ = CLI_COMMAND_CONNECT;
        return;
    }

    QString arg2 = args[2].toLower();
    if (isProtocol(arg2)) {
        protocol_ = arg2;
        cliCommand_ = CLI_COMMAND_CONNECT;
        return;
    }

    if (arg2 == "best") {
        locationStr_ = "best";
        cliCommand_ = CLI_COMMAND_CONNECT_BEST;
    } else {
        locationStr_ = arg2;
        cliCommand_ = CLI_COMMAND_CONNECT_LOCATION;
    }

    if (args.length() > 3) {
        QString arg3 = args[3].toLower();
        if (isProtocol(arg3)) {
            protocol_ = arg3;
        }
    }
}

void CliArguments::parseFirewall(const QStringList &args)
{
    if (args.length() <= 2) {
        cliCommand_ = CLI_COMMAND_HELP;
        return;
    }

    QString arg2 = args[2].toLower();
    if (arg2 == "on") {
        cliCommand_ = CLI_COMMAND_FIREWALL_ON;
    } else if (arg2 == "off") {
        cliCommand_ = CLI_COMMAND_FIREWALL_OFF;
    } else {
        cliCommand_ = CLI_COMMAND_HELP;
    }
}

void CliArguments::parseKeyLimit(const QStringList &args)
{
    if (args.length() <= 2) {
        cliCommand_ = CLI_COMMAND_HELP;
        return;
    }

    cliCommand_ = CLI_COMMAND_SET_KEYLIMIT_BEHAVIOR;
    QString arg2 = args[2].toLower();
    if (arg2 == "keep") {
        keyLimitDelete_ = false;
    } else if (arg2 == "delete") {
        keyLimitDelete_ = true;
    } else {
        cliCommand_ = CLI_COMMAND_HELP;
    }
}

void CliArguments::parseLogin(const QStringList &args)
{
    if (args.length() < 4) {
        cliCommand_ = CLI_COMMAND_HELP;
        return;
    }

    username_ = args.at(2);
    password_ = args.at(3);
    if (args.length() > 4) {
        code2fa_ = args.at(4);
    }

    cliCommand_ = CLI_COMMAND_LOGIN;
}

void CliArguments::parseLogout(const QStringList &args)
{
    if (args.length() < 2) {
        cliCommand_ = CLI_COMMAND_HELP;
        return;
    }

    cliCommand_ = CLI_COMMAND_LOGOUT;
    if (args.length() > 2 && args[2].toLower() == "on") {
        keepFirewallOn_ = true;
    } else {
        keepFirewallOn_ = false;
    }
}

void CliArguments::parsePreferences(const QStringList &args)
{
    if (args.length() <= 2) {
        cliCommand_ = CLI_COMMAND_HELP;
        return;
    }

    QString arg2 = args[2].toLower();
    if (arg2 == "reload") {
        cliCommand_ = CLI_COMMAND_RELOAD_CONFIG;
    } else {
        cliCommand_ = CLI_COMMAND_HELP;
    }
}

void CliArguments::parseLogs(const QStringList &args)
{
    if (args.length() <= 2) {
        cliCommand_ = CLI_COMMAND_HELP;
        return;
    }

    QString arg2 = args[2].toLower();
    if (arg2 == "send") {
        cliCommand_ = CLI_COMMAND_SEND_LOGS;
    } else {
        cliCommand_ = CLI_COMMAND_HELP;
    }
}

void CliArguments::processArguments()
{
    QStringList args = qApp->arguments();

    if (args.length() <= 1) {
        return;
    }

    QString arg1 = args[1].toLower();
    if (arg1 == "connect") {
        parseConnect(args);
    } else if (arg1 == "disconnect") {
        cliCommand_ = CLI_COMMAND_DISCONNECT;
    } else if (arg1 == "firewall") {
        parseFirewall(args);
    } else if (arg1 == "?" || arg1 == "help") {
        cliCommand_ = CLI_COMMAND_HELP;
    } else if (arg1 == "keylimit") {
        parseKeyLimit(args);
    } else if (arg1 == "locations") {
        cliCommand_ = CLI_COMMAND_LOCATIONS;
    } else if (arg1 == "login") {
        parseLogin(args);
    } else if (arg1 == "logout") {
        parseLogout(args);
    } else if (arg1 == "status") {
        cliCommand_ = CLI_COMMAND_STATUS;
#ifdef CLI_ONLY
    } else if (arg1 == "preferences") {
        parsePreferences(args);
#endif
    } else if (arg1 == "update") {
        cliCommand_ = CLI_COMMAND_UPDATE;
    } else if (arg1 == "logs") {
        parseLogs(args);
    }
}

CliCommand CliArguments::cliCommand() const
{
    return cliCommand_;
}

const QString &CliArguments::location() const
{
    return locationStr_;
}

const QString &CliArguments::username() const
{
    return username_;
}

const QString &CliArguments::password() const
{
    return password_;
}

const QString &CliArguments::code2fa() const
{
    return code2fa_;
}

const QString &CliArguments::protocol() const
{
    return protocol_;
}

bool CliArguments::keepFirewallOn() const
{
    return keepFirewallOn_;
}

bool CliArguments::keyLimitDelete() const
{
    return keyLimitDelete_;
}

