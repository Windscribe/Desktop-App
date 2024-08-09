#include <QCoreApplication>
#include <QDateTime>
#include <QObject>
#include <QProcess>

#include <iostream>

#include "backendcommander.h"
#include "cliarguments.h"
#include "languagecontroller.h"
#include "utils/languagesutil.h"
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

    LanguageController::instance().setLanguage(LanguagesUtil::systemLanguage());

    if (cliArgs.cliCommand() == CLI_COMMAND_NONE || cliArgs.cliCommand() == CLI_COMMAND_HELP) {
        qCDebug(LOG_BASIC) << "Printing help menu";
        std::cout << "windscribe-cli v" << WINDSCRIBE_VERSION_STR << std::endl;
        std::cout << std::endl;
        std::cout << "windscribe-cli <command>:" << std::endl;
        std::cout << std::endl;
        std::cout << "Authenticating with Windscribe" << std::endl;
        std::cout << "    login \"username\" \"password\" [2FA code]" << std::endl;
        std::cout << "        " << "Login with given username and password, and optional two-factor authentication code" << std::endl;
        std::cout << "    logout [on|off]" << std::endl;
        std::cout << "        " << "Sign out of the application, and optionally leave the firewall ON/OFF" << std::endl;
        std::cout << std::endl;
        std::cout << "Getting application state" << std::endl;
        std::cout << "    status" << std::endl;
        std::cout << "        " << "View basic login, connection, and account information" << std::endl;
        std::cout << "    locations" << std::endl;
        std::cout << "        " << "View a list of available locations" << std::endl;
        std::cout << std::endl;
#ifdef CLI_ONLY
        std::cout << "Managing preferences" << std::endl;
        std::cout << "    preferences reload" << std::endl;
        std::cout << "        " << "Reload the preference conf file (located at ~/.config/Windscribe/windscribe-cli.conf)" << std::endl;
        std::cout << std::endl;
#endif
        std::cout << "Managing VPN and firewall" << std::endl;
        std::cout << "    connect [protocol]" << std::endl;
        std::cout << "        " << "Connects to last connected location. If no last location, connect to best location." << std::endl;
        std::cout << "    connect best [protocol]" << std::endl;
        std::cout << "        " << "Connects to best location" << std::endl;
        std::cout << "    connect \"RegionName\" [protocol]" << std::endl;
        std::cout << "        " << "Connects to a random datacenter in the region" << std::endl;
        std::cout << "    connect \"CityName\" [protocol]" << std::endl;
        std::cout << "        " << "Connects to a random datacenter in the city" << std::endl;
        std::cout << "    connect \"Nickname\" [protocol]" << std::endl;
        std::cout << "        " << "Connects to datacenter with same nickname" << std::endl;
        std::cout << "    connect \"ISO CountryCode\" [protocol]" << std::endl;
        std::cout << "        " << "Connects to a random data center in the country" << std::endl;
        std::cout << "    keylimit keep|delete" << std::endl;
        std::cout << "        " << "Configures behavior for the next time during this run that the key limit is reached for WireGuard." << std::endl;
        std::cout << "        " << "keep [default] - Do not delete key.  Connection attempt will fail if the key limit is reached." << std::endl;
        std::cout << "        " << "delete - Delete the oldest WireGuard key if key limit is reached." << std::endl;
        std::cout << "    disconnect" << std::endl;
        std::cout << "        " << "Disconnects from current datacenter" << std::endl;
        std::cout << "    firewall on|off" << std::endl;
        std::cout << "        " << "Turn firewall ON/OFF" << std::endl;
        std::cout << std::endl;
#ifdef Q_OS_LINUX
        std::cout << "Protocols: " << "wireguard, udp, tcp, stealth, wstunnel" << std::endl;
#else
        std::cout << "Protocols: " << "wireguard, ikev2, udp, tcp, stealth, wstunnel" << std::endl;
#endif
        std::cout << std::endl;
        std::cout << "Application administration" << std::endl;
        std::cout << "    logs send" << std:: endl;
        std::cout << "        " << "Send debug log to Windscribe" << std::endl;
        std::cout << "    update" << std::endl;
        std::cout << "        " << "Updates to the latest available version" << std::endl;
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

    if (Utils::isAppAlreadyRunning()) {
        backendCommander->initAndSend();
    } else {
#ifdef CLI_ONLY
        uid_t uid = geteuid();
        if (uid == 0) {
            logAndCout("Error: this application should not run as root.");
            return -1;
        }

        int err = system("systemctl --user daemon-reload && systemctl --user enable windscribe && systemctl --user start windscribe");
        if (err != 0) {
            logAndCout("Could not start application");
            return err;
        }
        backendCommander->initAndSend();
#else
#ifdef Q_OS_WIN
        QString appPath = QCoreApplication::applicationDirPath() + "/Windscribe.exe";
#else
        QString appPath = QCoreApplication::applicationDirPath() + "/Windscribe";
#endif
        QString workingDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_WIN
        QProcess::startDetached(appPath, QStringList(), workingDir);
        backendCommander->initAndSend();
#else
        // use non-static start detached to prevent GUI output from polluting cli
        QProcess process;
        process.setProgram(appPath);
        process.setWorkingDirectory(workingDir);
        process.setStandardOutputFile(QProcess::nullDevice());
        process.setStandardErrorFile(QProcess::nullDevice());
        qint64 pid;
        process.startDetached(&pid);

        backendCommander->initAndSend();
#endif
#endif
    }

    return a.exec();
}

void logAndCout(const QString &str)
{
    if (str.isEmpty()) {
        return;
    }
    qCDebug(LOG_BASIC) << str;
    std::cout << str.toStdString() << std::endl;
}
