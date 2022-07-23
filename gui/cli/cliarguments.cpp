#include <QCoreApplication>
//#include <iostream>

#include "cliarguments.h"

CliArguments::CliArguments()
{
}

void CliArguments::processArguments()
{
    QStringList args = qApp->arguments();

    if (args.length() > 1)
    {
        QString arg1 = args[1].toLower();
        if (arg1 == "connect")
        {
            if (args.length() > 2)
            {
                QString arg2 = args[2].toLower();
                if (arg2 == "best")
                {
                    locationStr_ = "best";
                    cliCommand_ = CLI_COMMAND_CONNECT_BEST;
                }
                else
                {
                    locationStr_ = arg2;
                    cliCommand_ = CLI_COMMAND_CONNECT_LOCATION;
                }
            }
            else
            {
                cliCommand_ = CLI_COMMAND_CONNECT;
            }
        }
        else if (arg1 == "disconnect")
        {
            cliCommand_ = CLI_COMMAND_DISCONNECT;
        }
        else if (arg1 == "firewall")
        {
            if (args.length() > 2)
            {
                QString arg2 = args[2].toLower();
                if (arg2 == "on")
                {
                    cliCommand_ = CLI_COMMAND_FIREWALL_ON;
                }
                else if (arg2 == "off")
                {
                    cliCommand_ = CLI_COMMAND_FIREWALL_OFF;
                }
            }
        }
        else if (arg1 == "?" || arg1 == "help")
        {
            cliCommand_ = CLI_COMMAND_HELP;
        }
        else if (arg1 == "locations")
        {
            cliCommand_ = CLI_COMMAND_LOCATIONS;
        }
        else if (arg1 == "login")
        {
            if (args.length() >= 4)
            {
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
        }
        else if (arg1 == "signout")
        {
            cliCommand_ = CLI_COMMAND_SIGN_OUT;

            if (args.length() > 2) {
                keepFirewallOn_ = (args[2].toLower() == "on");
            }
            else {
                keepFirewallOn_ = false;
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

bool CliArguments::keepFirewallOn() const
{
    return keepFirewallOn_;
}
