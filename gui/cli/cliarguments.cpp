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


void CliArguments::processArguments()
{
    QStringList args = qApp->arguments();

    if (args.length() > 1) {
        QString arg1 = args[1].toLower();
        if (arg1 == "connect") {
            if (args.length() > 2) {
                QString arg2 = args[2].toLower();
                if (isProtocol(arg2)) {
                    protocol_ = arg2;
                    cliCommand_ = CLI_COMMAND_CONNECT;
                } else {
                    if (arg2 == "best") {
                        locationStr_ = "best";
                        cliCommand_ = CLI_COMMAND_CONNECT_BEST;
                    } else {
                        locationStr_ = arg2;
                        cliCommand_ = CLI_COMMAND_CONNECT_LOCATION;
                    }

                    if (args.length() > 3 && isProtocol(args[3].toLower())) {
                        protocol_ = args[3].toLower();
                    }
                }
            } else {
                cliCommand_ = CLI_COMMAND_CONNECT;
            }
        } else if (arg1 == "disconnect") {
            cliCommand_ = CLI_COMMAND_DISCONNECT;
        } else if (arg1 == "firewall") {
            if (args.length() > 2) {
                QString arg2 = args[2].toLower();
                if (arg2 == "on") {
                    cliCommand_ = CLI_COMMAND_FIREWALL_ON;
                } else if (arg2 == "off") {
                    cliCommand_ = CLI_COMMAND_FIREWALL_OFF;
                }
            }
        } else if (arg1 == "?" || arg1 == "help") {
            cliCommand_ = CLI_COMMAND_HELP;
        } else if (arg1 == "locations") {
            cliCommand_ = CLI_COMMAND_LOCATIONS;
        } else if (arg1 == "login") {
            if (args.length() >= 4) {
                username_ = args.at(2);
                password_ = args.at(3);
                if (args.length() > 4) {
                    code2fa_ = args.at(4);
                }

                //std::cout << "username:" << qPrintable(username_) << std::endl;
                //std::cout << "password:" << qPrintable(password_) << std::endl;
                //std::cout << "2FA:" << qPrintable(code2fa_) << std::endl;

                cliCommand_ = CLI_COMMAND_LOGIN;
            }
        } else if (arg1 == "logout") {
            cliCommand_ = CLI_COMMAND_LOGOUT;

            if (args.length() > 2) {
                keepFirewallOn_ = (args[2].toLower() == "on");
            } else {
                keepFirewallOn_ = false;
            }
        } else if (arg1 == "status") {
            cliCommand_ = CLI_COMMAND_STATUS;
#ifdef CLI_ONLY
        } else if (arg1 == "preferences") {
            if (args.length() > 2) {
                QString arg2 = args[2].toLower();
                if (arg2 == "reload") {
                    cliCommand_ = CLI_COMMAND_RELOAD_CONFIG;
                } else {
                    cliCommand_ = CLI_COMMAND_HELP;
                }
            } else {
                cliCommand_ = CLI_COMMAND_HELP;
            }
#endif
        } else if (arg1 == "update") {
            cliCommand_ = CLI_COMMAND_UPDATE;
        } else if (arg1 == "logs") {
            if (args.length() > 2) {
                QString arg2 = args[2].toLower();
                if (arg2 == "send") {
                    cliCommand_ = CLI_COMMAND_SEND_LOGS;
                } else {
                    cliCommand_ = CLI_COMMAND_HELP;
                }
            } else {
                cliCommand_ = CLI_COMMAND_HELP;
            }
        }
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

