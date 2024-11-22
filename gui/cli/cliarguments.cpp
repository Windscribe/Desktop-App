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

static QString getOptions(const QString &str)
{
    if (str.startsWith("-")) {
        return str.mid(1);
    }

    return "";
}

void CliArguments::setUsername(const QString &username)
{
    username_ = username;
}

void CliArguments::setPassword(const QString &password)
{
    password_ = password;
}

void CliArguments::set2FACode(const QString &code)
{
    code2fa_ = code;
}

void CliArguments::parseConnect(const QStringList &args)
{
    if (args.length() <= 2) {
        cliCommand_ = CLI_COMMAND_CONNECT;
        return;
    }

    int idx = 2;
    QString arg = args[idx].toLower();
    if (getOptions(arg) == "n") {
        nonBlocking_ = true;
        idx++;
    }

    if (args.length() <= idx) {
        cliCommand_ = CLI_COMMAND_CONNECT;
        return;
    }
    arg = args[idx++].toLower();
    if (isProtocol(arg)) {
        protocol_ = arg;
        cliCommand_ = CLI_COMMAND_CONNECT;
        return;
    }

    if (arg == "best") {
        locationStr_ = "best";
        cliCommand_ = CLI_COMMAND_CONNECT_BEST;
    } else if (arg == "static") {
        if (args.length() <= idx) {
            cliCommand_ = CLI_COMMAND_HELP;
            return;
        }
        locationStr_ = args[idx++];
        cliCommand_ = CLI_COMMAND_CONNECT_STATIC;
    } else {
        locationStr_ = arg;
        cliCommand_ = CLI_COMMAND_CONNECT_LOCATION;
    }

    if (args.length() > idx) {
        arg = args[idx].toLower();
        if (isProtocol(arg)) {
            protocol_ = arg;
        }
    }
}

void CliArguments::parseDisconnect(const QStringList &args)
{
    cliCommand_ = CLI_COMMAND_DISCONNECT;
    if (args.length() <= 2) {
        return;
    }

    int idx = 2;
    QString arg = args[idx].toLower();
    if (getOptions(arg) == "n") {
        nonBlocking_ = true;
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

void CliArguments::parseLocations(const QStringList &args)
{
    if (args.length() <= 2) {
        cliCommand_ = CLI_COMMAND_LOCATIONS;
        return;
    }

    QString arg2 = args[2].toLower();
    if (arg2 == "static") {
        cliCommand_ = CLI_COMMAND_LOCATIONS_STATIC;
    }
}

void CliArguments::parseLogin(const QStringList &args)
{
    cliCommand_ = CLI_COMMAND_LOGIN;

    if (args.length() <= 2) {
        return;
    }

    int idx = 2;
    QString arg;
    QString opt;

    do {
        arg = args[idx].toLower();
        opt = getOptions(arg);

        if (opt == "n") {
            nonBlocking_ = true;
            idx++;
        } else if (opt == "2") {
            need2FA_ = true;
            idx++;
        } else if (!opt.isEmpty()) {
            cliCommand_ = CLI_COMMAND_HELP;
            return;
        }
    } while (!opt.isEmpty() && args.length() > idx);

    if (args.length() > idx) {
        // More positional arguments exist, consider it 'username'
        username_ = args.at(idx++);
    }

    if (args.length() > idx) {
        // More positional arguments exist, consider it 'password'
        password_ = args.at(idx++);
    }
}

void CliArguments::parseLogout(const QStringList &args)
{
    if (args.length() < 2) {
        cliCommand_ = CLI_COMMAND_HELP;
        return;
    }

    cliCommand_ = CLI_COMMAND_LOGOUT;

    if (args.length() == 2) {
        return;
    }

    int idx = 2;
    QString arg = args[idx].toLower();
    if (getOptions(arg) == "n") {
        nonBlocking_ = true;
        idx++;
    }

    if (args.length() > idx && args[idx].toLower() == "on") {
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
        parseDisconnect(args);
    } else if (arg1 == "firewall") {
        parseFirewall(args);
    } else if (arg1 == "?" || arg1 == "help") {
        cliCommand_ = CLI_COMMAND_HELP;
#ifdef CLI_ONLY
    } else if (arg1 == "keylimit") {
        parseKeyLimit(args);
#endif
    } else if (arg1 == "locations") {
        parseLocations(args);
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

bool CliArguments::nonBlocking() const
{
    switch(cliCommand_) {
        case CLI_COMMAND_CONNECT:
        case CLI_COMMAND_CONNECT_BEST:
        case CLI_COMMAND_CONNECT_LOCATION:
        case CLI_COMMAND_CONNECT_STATIC:
        case CLI_COMMAND_DISCONNECT:
        case CLI_COMMAND_LOGIN:
        case CLI_COMMAND_LOGOUT:
            return nonBlocking_;
        case CLI_COMMAND_UPDATE:
            return false;
        default:
            return true;
    }
}

bool CliArguments::need2FA() const
{
    return need2FA_;
}
