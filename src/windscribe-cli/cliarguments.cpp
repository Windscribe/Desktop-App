#include <QCoreApplication>
#include <algorithm>

#include "cliarguments.h"
#include "help.h"

CliArguments::CliArguments()
{
}

static bool isProtocol(const QString &str)
{
    QString sub = str.split(':').first();

    if (sub == "wireguard" || sub == "ikev2" || sub == "udp" || sub == "tcp" || sub == "stealth" || sub == "wstunnel") {
        return true;
    }
    return false;
}

static bool isHelpFlag(const QString &str)
{
    return str == "-h" || str == "--help";
}

void CliArguments::setError(const QString &command, const QString &error)
{
    cliCommand_ = CLI_COMMAND_INVALID;
    helpTopic_ = command;
    error_ = error;
}

// Parses an optional "protocol[:port]" argument.  Returns false, with the error set, if it is not one.
bool CliArguments::parseProtocol(const QString &command, const QString &arg)
{
    const QString str = arg.toLower();
    if (!isProtocol(str)) {
        setError(command, QString("unknown protocol \"%1\" (quote a location that contains spaces)").arg(arg));
        return false;
    }

    const QStringList parts = str.split(':');
    if (parts.size() > 2) {
        setError(command, QString("invalid protocol and port \"%1\"").arg(arg));
        return false;
    }
    if (parts.size() == 2) {
        bool ok = false;
        port_ = parts[1].toUInt(&ok);
        if (!ok || port_ == 0 || port_ > 65535) {
            setError(command, QString("invalid port \"%1\"").arg(parts[1]));
            return false;
        }
    }
    protocol_ = parts[0];
    return true;
}

void CliArguments::parseConnect(const QStringList &args)
{
    QStringList positional;
    for (const QString &arg : args) {
        if (arg == "-n" || arg == "-N") {
            nonBlocking_ = true;
        } else if (arg.startsWith("-")) {
            setError("connect", QString("unknown flag \"%1\"").arg(arg));
            return;
        } else {
            positional << arg;
        }
    }

    int idx = 0;
    cliCommand_ = CLI_COMMAND_CONNECT;
    if (positional.isEmpty()) {
        return;
    }

    const QString first = positional[idx].toLower();
    if (isProtocol(first)) {
        // Last location with the given protocol.
    } else if (first == "best") {
        locationStr_ = "best";
        cliCommand_ = CLI_COMMAND_CONNECT_BEST;
        idx++;
    } else if (first == "static") {
        if (positional.length() < 2) {
            setError("connect", "missing static IP city or IP");
            return;
        }
        // Static IP locations are matched exactly, so keep the case.
        locationStr_ = positional[1];
        cliCommand_ = CLI_COMMAND_CONNECT_STATIC;
        idx += 2;
    } else {
        locationStr_ = first;
        cliCommand_ = CLI_COMMAND_CONNECT_LOCATION;
        idx++;
    }

    if (positional.length() > idx && !parseProtocol("connect", positional[idx++])) {
        return;
    }
    if (positional.length() > idx) {
        setError("connect", QString("unexpected argument \"%1\"").arg(positional[idx]));
    }
}

void CliArguments::parseDisconnect(const QStringList &args)
{
    cliCommand_ = CLI_COMMAND_DISCONNECT;
    for (const QString &arg : args) {
        if (arg == "-n" || arg == "-N") {
            nonBlocking_ = true;
        } else {
            setError("disconnect", QString("unexpected argument \"%1\"").arg(arg));
            return;
        }
    }
}

void CliArguments::parseFirewall(const QStringList &args)
{
    const QString arg = args.value(0).toLower();
    if (args.length() != 1 || (arg != "on" && arg != "off")) {
        setError("firewall", "expected \"on\" or \"off\"");
    } else if (arg == "on") {
        cliCommand_ = CLI_COMMAND_FIREWALL_ON;
    } else {
        cliCommand_ = CLI_COMMAND_FIREWALL_OFF;
    }
}

void CliArguments::parseIp(const QStringList &args)
{
    const QString arg = args.value(0).toLower();
    if (arg == "rotate" && args.length() == 1) {
        cliCommand_ = CLI_COMMAND_IP_ROTATE;
    } else if (arg == "fav" && args.length() == 1) {
        cliCommand_ = CLI_COMMAND_IP_FAV;
    } else if (arg == "unfav" && args.length() == 2) {
        ipAddress_ = args[1];
        cliCommand_ = CLI_COMMAND_IP_UNFAV;
    } else if (arg == "unfav" && args.length() == 1) {
        setError("ip", "missing IP address");
    } else {
        setError("ip", "expected \"rotate\", \"fav\", or \"unfav <ip>\"");
    }
}

void CliArguments::parseKeyLimit(const QStringList &args)
{
    const QString arg = args.value(0).toLower();
    if (args.length() != 1 || (arg != "keep" && arg != "delete")) {
        setError("keylimit", "expected \"keep\" or \"delete\"");
        return;
    }
    cliCommand_ = CLI_COMMAND_SET_KEYLIMIT_BEHAVIOR;
    keyLimitDelete_ = (arg == "delete");
}

void CliArguments::parseIgnoreSslErrors(const QStringList &args)
{
    const QString arg = args.value(0).toLower();
    if (args.length() != 1 || (arg != "on" && arg != "off")) {
        setError("ignoresslerrors", "expected \"on\" or \"off\"");
        return;
    }
    cliCommand_ = CLI_COMMAND_SET_IGNORE_SSL_ERRORS;
    ignoreSslErrors_ = (arg == "on");
}

void CliArguments::parseLocations(const QStringList &args)
{
    const QString arg = args.value(0).toLower();
    if (args.isEmpty()) {
        cliCommand_ = CLI_COMMAND_LOCATIONS;
    } else if (arg == "fav" && args.length() == 1) {
        cliCommand_ = CLI_COMMAND_LOCATIONS_FAV;
    } else if (arg == "static" && args.length() == 1) {
        cliCommand_ = CLI_COMMAND_LOCATIONS_STATIC;
    } else {
        setError("locations", "expected nothing, \"fav\", or \"static\"");
    }
}

void CliArguments::parsePorts(const QStringList &args)
{
    if (args.isEmpty()) {
        setError("ports", "missing protocol");
        return;
    }
    if (args.length() > 1) {
        setError("ports", QString("unexpected argument \"%1\"").arg(args[1]));
        return;
    }

    const QString arg = args[0].toLower();
    if (!isProtocol(arg) || arg.contains(':')) {
        setError("ports", QString("unknown protocol \"%1\"").arg(args[0]));
        return;
    }
    protocol_ = arg;
    cliCommand_ = CLI_COMMAND_PORTS;
}

void CliArguments::parseLogin(const QStringList &args)
{
    if (args.length() > 2) {
        setError("login", "too many arguments; quote a username or password that contains spaces");
        return;
    }
    cliCommand_ = CLI_COMMAND_LOGIN;
    username_ = args.value(0);
    password_ = args.value(1);
}

void CliArguments::parseLogout(const QStringList &args)
{
    const QString arg = args.value(0).toLower();
    if (args.length() > 1 || (!args.isEmpty() && arg != "on" && arg != "off")) {
        setError("logout", "expected nothing, \"on\", or \"off\"");
        return;
    }
    cliCommand_ = CLI_COMMAND_LOGOUT;
    keepFirewallOn_ = (arg == "on");
}

void CliArguments::parsePreferences(const QStringList &args)
{
    if (args.length() == 1 && args[0].toLower() == "reload") {
        cliCommand_ = CLI_COMMAND_RELOAD_CONFIG;
    } else {
        setError("", "unknown command \"preferences\"");
    }
}

void CliArguments::parseLogs(const QStringList &args)
{
    if (args.length() == 1 && args[0].toLower() == "send") {
        cliCommand_ = CLI_COMMAND_SEND_LOGS;
    } else {
        setError("logs", "expected \"send\"");
    }
}

void CliArguments::processArguments()
{
    QStringList args = qApp->arguments();

    if (args.length() <= 1) {
        return;
    }

    const QString command = args[1].toLower();
    const QStringList rest = args.mid(2);

    if (command == "help" || command == "?" || isHelpFlag(command)) {
        cliCommand_ = CLI_COMMAND_HELP;
        if (!rest.isEmpty()) {
            // "help --help" is help about the help command.
            helpTopic_ = isHelpFlag(rest[0]) ? QString("help") : rest[0].toLower();
            if (!Help::isCommand(helpTopic_)) {
                setError("", QString("unknown command \"%1\"").arg(rest[0]));
            }
        }
        return;
    }

    if (command == "version" || command == "-v" || command == "--version") {
        if (std::any_of(rest.begin(), rest.end(), isHelpFlag)) {
            cliCommand_ = CLI_COMMAND_HELP;
            helpTopic_ = "version";
        } else if (!rest.isEmpty()) {
            setError("version", QString("unexpected argument \"%1\"").arg(rest[0]));
        } else {
            cliCommand_ = CLI_COMMAND_VERSION;
        }
        return;
    }

    // "<command> --help".  A username or password could look like a flag, so for login only the first argument counts.
    if (Help::isCommand(command)) {
        const bool wantsHelp = (command == "login") ? isHelpFlag(rest.value(0)) : std::any_of(rest.begin(), rest.end(), isHelpFlag);
        if (wantsHelp) {
            cliCommand_ = CLI_COMMAND_HELP;
            helpTopic_ = command;
            return;
        }
    }

    if (command == "connect") {
        parseConnect(rest);
    } else if (command == "disconnect") {
        parseDisconnect(rest);
    } else if (command == "firewall") {
        parseFirewall(rest);
#ifdef CLI_ONLY
    } else if (command == "keylimit") {
        parseKeyLimit(rest);
    } else if (command == "ignoresslerrors") {
        parseIgnoreSslErrors(rest);
    } else if (command == "preferences") {
        parsePreferences(rest);
#endif
    } else if (command == "ip") {
        parseIp(rest);
    } else if (command == "locations") {
        parseLocations(rest);
    } else if (command == "ports") {
        parsePorts(rest);
    } else if (command == "login") {
        parseLogin(rest);
    } else if (command == "logout") {
        parseLogout(rest);
    } else if (command == "logs") {
        parseLogs(rest);
    } else if (command == "status" || command == "update" || command == "amneziawg") {
        if (!rest.isEmpty()) {
            setError(command, QString("unexpected argument \"%1\"").arg(rest[0]));
        } else if (command == "status") {
            cliCommand_ = CLI_COMMAND_STATUS;
        } else if (command == "update") {
            cliCommand_ = CLI_COMMAND_UPDATE;
        } else {
            cliCommand_ = CLI_COMMAND_AMNEZIAWG;
        }
    } else {
        setError("", QString("unknown command \"%1\"").arg(args[1]));
    }
}

const QString &CliArguments::helpTopic() const
{
    return helpTopic_;
}

const QString &CliArguments::error() const
{
    return error_;
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

const QString &CliArguments::protocol() const
{
    return protocol_;
}

const uint &CliArguments::port() const
{
    return port_;
}

const QString &CliArguments::ipAddress() const
{
    return ipAddress_;
}

bool CliArguments::keepFirewallOn() const
{
    return keepFirewallOn_;
}

bool CliArguments::keyLimitDelete() const
{
    return keyLimitDelete_;
}

bool CliArguments::ignoreSslErrors() const
{
    return ignoreSslErrors_;
}

bool CliArguments::nonBlocking() const
{
    switch(cliCommand_) {
        case CLI_COMMAND_CONNECT:
        case CLI_COMMAND_CONNECT_BEST:
        case CLI_COMMAND_CONNECT_LOCATION:
        case CLI_COMMAND_CONNECT_STATIC:
        case CLI_COMMAND_DISCONNECT:
            return nonBlocking_;
        case CLI_COMMAND_LOGIN:
        case CLI_COMMAND_LOGOUT:
        case CLI_COMMAND_UPDATE:
            return false;
        default:
            return true;
    }
}
