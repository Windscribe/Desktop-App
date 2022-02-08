#include "cliapplication.h"

#include <QStringList>

CliApplication::CliApplication(int &argc, char **argv) : QCoreApplication (argc, argv)
    ,locationStr_("")
{

}

CliCommand CliApplication::cliCommand()
{
    CliCommand command = CLI_COMMAND_NONE;

    QStringList args = arguments();

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
                    command = CLI_COMMAND_CONNECT_BEST;
                }
                else
                {
                    locationStr_ = arg2;
                    command = CLI_COMMAND_CONNECT_LOCATION;
                }
            }
            else
            {
                command = CLI_COMMAND_CONNECT;
            }
        }
        else if (arg1 == "disconnect")
        {
            command = CLI_COMMAND_DISCONNECT;
        }
        else if (arg1 == "firewall")
        {
            if (args.length() > 2)
            {
                QString arg2 = args[2].toLower();
                if (arg2 == "on")
                {
                    command = CLI_COMMAND_FIREWALL_ON;
                }
                else if (arg2 == "off")
                {
                    command = CLI_COMMAND_FIREWALL_OFF;
                }
            }
        }
        else if (arg1 == "?" || arg1 == "help")
        {
            command = CLI_COMMAND_HELP;
        }
        else if (arg1 == "locations")
        {
            command = CLI_COMMAND_LOCATIONS;
        }
    }

    return command;
}

const QString &CliApplication::location()
{
    return locationStr_;
}
