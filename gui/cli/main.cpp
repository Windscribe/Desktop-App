#include <QCoreApplication>
#include <QDateTime>
#include <QObject>
#include <QProcess>

#include <iostream>

#include "backendcommander.h"
#include "cliarguments.h"
#include "languagecontroller.h"
#include "utils.h"
#include "utils/languagesutil.h"
#include "utils/log/logger.h"
#include "utils/log/paths.h"
#include "utils/utils.h"

#ifdef Q_OS_LINUX
    #include <semaphore.h>
    #include <time.h>
    #include <unistd.h>
#elif defined(Q_OS_MACOS)
    #include <semaphore.h>
    #include <time.h>
    #include <unistd.h>
#else
    #include "utils/winutils.h"
#endif

// TODO: add support for config nodes

// TODO: there is crash that occurs when the cli is closing down after completing its command
// Only seems to happen when debugger is attached to CLI
// (observation made with GUI and Engine each running in Qt Creator instances in debug mode)

void logAndCout(const QString & str);

int main(int argc, char *argv[])
{
    if (Utils::isCLIRunning(1)) {
        logAndCout("Windscribe CLI is already running");
        return 1;
    }

    // clear Qt plugin library paths for release build
#ifndef QT_DEBUG
    QCoreApplication::setLibraryPaths(QStringList());
#endif

    QCoreApplication::setApplicationName("Windscribe2");
    QCoreApplication::setOrganizationName("Windscribe");

    log_utils::Logger::instance().install(log_utils::paths::cliLogLocation(), false);

    qCInfo(LOG_CLI) << "=== Started ===";
    qCInfo(LOG_CLI) << "OS Version:" << Utils::getOSVersion();

    QCoreApplication a(argc, argv);

    CliArguments cliArgs;
    cliArgs.processArguments();

    LanguageController::instance().setLanguage(LanguagesUtil::systemLanguage());

    if (cliArgs.cliCommand() == CLI_COMMAND_NONE || cliArgs.cliCommand() == CLI_COMMAND_HELP) {
        qCDebug(LOG_CLI) << "Printing help menu";
        std::cout << "windscribe-cli v" << WINDSCRIBE_VERSION_STR << std::endl;
        std::cout << std::endl;
        std::cout << "windscribe-cli <command>:" << std::endl;
        std::cout << std::endl;
        std::cout << "Authenticating with Windscribe" << std::endl;
        std::cout << "    login [username [password]]" << std::endl;
        std::cout << "        " << "Login with given credentials." << std::endl;
        std::cout << "        " << "If any input has special characters in it, it should be wrapped in single quotes or escaped." << std::endl;
        std::cout << "        " << "If any credentials are not provided, a prompt will be presented. 2FA code is always prompted if required." << std::endl;
        std::cout << "    logout [on|off]" << std::endl;
        std::cout << "        " << "Sign out of the application, and optionally leave the firewall ON/OFF" << std::endl;
        std::cout << std::endl;
        std::cout << "Getting application state" << std::endl;
        std::cout << "    status" << std::endl;
        std::cout << "        " << "View basic login, connection, and account information" << std::endl;
        std::cout << "    locations [static]" << std::endl;
        std::cout << "        " << "View a list of available locations. If 'static' is present, show static IP locations instead" << std::endl;
        std::cout << std::endl;
#ifdef CLI_ONLY
        std::cout << "Managing preferences" << std::endl;
        std::cout << "    preferences reload" << std::endl;
        std::cout << "        " << "Reload the preference conf file (located at ~/.config/Windscribe/windscribe_cli.conf)" << std::endl;
        std::cout << std::endl;
#endif
        std::cout << "Managing VPN and firewall" << std::endl;
        std::cout << "    connect [options] [protocol]" << std::endl;
        std::cout << "        " << "Connect to last connected location. If no last location, connect to best location." << std::endl;
        std::cout << "    connect [options] best [protocol]" << std::endl;
        std::cout << "        " << "Connect to best location" << std::endl;
        std::cout << "    connect [options] \"ISO CountryCode\" [protocol]" << std::endl;
        std::cout << "    connect [options] \"RegionName\" [protocol]" << std::endl;
        std::cout << "    connect [options] \"CityName\" [protocol]" << std::endl;
        std::cout << "    connect [options] \"Nickname\" [protocol]" << std::endl;
        std::cout << "        " << "Connect to a random datacenter in the specified country, region, or city, or by nickname." << std::endl;
        std::cout << "    connect [options] static \"CityName\" [protocol]" << std::endl;
        std::cout << "    connect [options] static \"IP\" [protocol]" << std::endl;
        std::cout << "        " << "Connect to a static IP location by city or by IP." << std::endl;
#ifdef CLI_ONLY
        std::cout << "    keylimit keep|delete" << std::endl;
        std::cout << "        " << "Configure behaviour for the next time during this run that the key limit is reached for WireGuard." << std::endl;
        std::cout << "        " << "keep [default] - Do not delete key.  Connection attempt will fail if the key limit is reached." << std::endl;
        std::cout << "        " << "delete - Delete the oldest WireGuard key if key limit is reached." << std::endl;
#endif
        std::cout << "    disconnect [options]" << std::endl;
        std::cout << "        " << "Disconnect from current datacenter" << std::endl;
        std::cout << "    firewall on|off" << std::endl;
        std::cout << "        " << "Turn firewall on/off" << std::endl;
        std::cout << std::endl;
#ifdef Q_OS_LINUX
        std::cout << "Protocols: " << "wireguard, udp, tcp, stealth, wstunnel" << std::endl;
#else
        std::cout << "Protocols: " << "wireguard, ikev2, udp, tcp, stealth, wstunnel" << std::endl;
#endif
        std::cout << "Options: " << std::endl;
        std::cout << "    -n: non-blocking mode. Command returns immediately instead of waiting to complete." << std::endl;
        std::cout << std::endl;
        std::cout << "Application administration" << std::endl;
        std::cout << "    logs send" << std:: endl;
        std::cout << "        " << "Send debug log to Windscribe" << std::endl;
        std::cout << "    update" << std::endl;
        std::cout << "        " << "Update to the latest available version" << std::endl;
        return 0;
    }

    // If logging in, prompt for any missing credentials
    if (cliArgs.cliCommand() == CLI_COMMAND_LOGIN) {
        if (cliArgs.username().isEmpty()) {
            cliArgs.setUsername(Utils::getInput("Username: ", false));
        }
        if (cliArgs.password().isEmpty()) {
            cliArgs.setPassword(Utils::getInput("Password: ", true));
        }
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
    qCInfo(LOG_CLI) << str;
    std::cout << str.toStdString() << std::endl;
}

