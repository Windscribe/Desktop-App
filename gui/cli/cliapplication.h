#ifndef CLIAPPLICATION_H
#define CLIAPPLICATION_H

#include <QCoreApplication>

enum CliCommand { CLI_COMMAND_NONE, CLI_COMMAND_HELP ,
                  CLI_COMMAND_CONNECT, CLI_COMMAND_CONNECT_BEST, CLI_COMMAND_CONNECT_LOCATION,
                  CLI_COMMAND_DISCONNECT,
                  CLI_COMMAND_FIREWALL_ON, CLI_COMMAND_FIREWALL_OFF,
                  CLI_COMMAND_LOCATIONS };

class CliApplication : public QCoreApplication
{
    Q_OBJECT
public:
    CliApplication(int &argc, char **argv);

    CliCommand cliCommand();
    const QString &location();

private:
    QString locationStr_;
};

#endif // CLIAPPLICATION_H
