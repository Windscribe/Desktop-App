#include <QCoreApplication>
#include <QDateTime>
#include <QObject>
#include <QProcess>

#include <iostream>

#include "backendcommander.h"
#include "cliarguments.h"
#include "utils/logger.h"
#include "utils/utils.h"

#ifndef Q_OS_WIN
    #include <semaphore.h>
    #include <time.h>
    #include <unistd.h>
#endif

// TODO: add translations
// TODO: add support for config nodes

// TODO: there is crash that occurs when the cli is closing down after completing its command
// Only seems to happen when debugger is attached to CLI
// (observation made with GUI and Engine each running in Qt Creator instances in debug mode)

void logAndCout(const QString & str);

int main(int argc, char *argv[])
{
    // clear Qt plugin library paths for release build
#ifndef QT_DEBUG
    QCoreApplication::setLibraryPaths(QStringList());
#endif

    QCoreApplication::setApplicationName("Windscribe");
    QCoreApplication::setOrganizationName("Windscribe");

    Logger::instance().install("cli", false, false);

    qCDebug(LOG_BASIC) << "CLI start time:" << QDateTime::currentDateTime().toString();
    qCDebug(LOG_BASIC) << "OS Version:" << Utils::getOSVersion();

    QCoreApplication a(argc, argv);

    CliArguments cliArgs;
    cliArgs.processArguments();

    if (cliArgs.cliCommand() == CLI_COMMAND_NONE)
    {
        qCDebug(LOG_BASIC) << "CLI args fail: Couldn't determine appropriate command from arguments";
        QString output = QObject::tr("There appears to be an issue with the provided arguments. Try 'windscribe-cli help' to see available options");
        std::cout << output.toStdString() << std::endl;
        return 1;
    }
    else if (cliArgs.cliCommand() == CLI_COMMAND_HELP)
    {
        qCDebug(LOG_BASIC) << "Printing help menu";
        std::cout << "windscribe-cli supports the following commands:" << std::endl;
        std::cout << "connect                     - Connects to last connected location. If no last location, connect to best location." << std::endl;
        std::cout << "connect best                - Connects to best location" << std::endl;
        std::cout << "connect \"RegionName\"        - Connects to a random datacenter in the region" << std::endl;
        std::cout << "connect \"CityName\"          - Connects to a random datacenter in the city" << std::endl;
        std::cout << "connect \"Nickname\"          - Connects to datacenter with same nickname" << std::endl;
        std::cout << "connect \"ISO CountryCode\"   - Connects to a random data center in the country" << std::endl;
        std::cout << "disconnect                  - Disconnects from current datacenter" << std::endl;
        std::cout << "firewall on|off             - Turn firewall ON/OFF" << std::endl;
        std::cout << "locations                   - View a list of available locations" << std::endl;
        std::cout << "login \"username\" \"password\" [2FA code] - login with given username and password, and optional two-factor authentication code" << std::endl;
        std::cout << "signout [on|off]            - Sign out of the application, and optionally leave the firewall ON/OFF" << std::endl;
        std::cout << "status                      - View the connected/disconnected state of the application" << std::endl;
        return 0;
    }

    BackendCommander *backendCommander = new BackendCommander(cliArgs);

    QObject::connect(backendCommander, &BackendCommander::finished, [&](int returnCode, const QString &msg) {
        logAndCout(msg);
        backendCommander->deleteLater();
        backendCommander = nullptr;
        a.exit(returnCode);
    });

    QObject::connect(backendCommander, &BackendCommander::report, [&](const QString &msg) {
        logAndCout(msg);
    });

    if (Utils::isGuiAlreadyRunning())
    {
        qCDebug(LOG_BASIC) << "GUI detected -- attempting Engine connect";
        backendCommander->initAndSend(true);
    }
    else
    {
        if (cliArgs.cliCommand() == CLI_COMMAND_SIGN_OUT)
        {
            logAndCout(QCoreApplication::tr("The application is not running, signout not required."));
            return 0;
        }

        logAndCout(QCoreApplication::tr("No GUI instance detected, starting one now..."));

        // GUI pathing
#ifdef Q_OS_WIN
        QString guiPath = QCoreApplication::applicationDirPath() + "/Windscribe.exe";
#else
        QString guiPath = QCoreApplication::applicationDirPath() + "/Windscribe";
#endif
        QString workingDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_WIN
        QProcess::startDetached(guiPath, QStringList(), workingDir);
        backendCommander->initAndSend(false);
#else
        // use non-static start detached to prevent GUI output from polluting cli
        QProcess process;
        process.setProgram(guiPath);
        process.setWorkingDirectory(workingDir);
        process.setStandardOutputFile(QProcess::nullDevice());
        process.setStandardErrorFile(QProcess::nullDevice());
        qint64 pid;
        process.startDetached(&pid);

        backendCommander->initAndSend(false);
#endif
    }

    return a.exec();
}

void logAndCout(const QString &str)
{
    qCDebug(LOG_BASIC) << str;
    std::cout << str.toStdString() << std::endl;
}
