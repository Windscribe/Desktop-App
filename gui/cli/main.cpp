#include <QCoreApplication>
#include <QDateTime>
#include <QObject>
#include <QProcess>

#include <iostream>

#include "backendcommander.h"
#include "cliarguments.h"
#include "languagecontroller.h"
#include "utils/languagesutil.h"
#include "utils/log/logger.h"
#include "utils/log/paths.h"
#include "utils/utils.h"

#ifdef Q_OS_LINUX
    #include <semaphore.h>
    #include <termios.h>
    #include <time.h>
    #include <unistd.h>
#elif defined(Q_OS_MACOS)
    #include <readpassphrase.h>
    #include <semaphore.h>
    #include <time.h>
    #include <unistd.h>
#else
    #include <windows.h>
    #include "utils/winutils.h"
#endif

// TODO: add support for config nodes

// TODO: there is crash that occurs when the cli is closing down after completing its command
// Only seems to happen when debugger is attached to CLI
// (observation made with GUI and Engine each running in Qt Creator instances in debug mode)

void logAndCout(const QString & str);

static QString getInput(const QString &prompt, bool disableEcho)
{
#if defined(Q_OS_WIN)
    DWORD mode;
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 

    if (disableEcho) {
        GetConsoleMode(hStdin, &mode);
        SetConsoleMode(hStdin, mode & ~ENABLE_ECHO_INPUT);
    }

    std::string input;
    std::cout << prompt.toStdString();
    std::cin >> input;

    if (disableEcho) {
        std::cout << std::endl;
        SetConsoleMode(hStdin, mode);
    }

    return QString::fromStdString(input);
#elif defined(Q_OS_MAC)
    char buf[1024];
    memset(buf, 0, 1024);

    char *ret = readpassphrase(prompt.toStdString().c_str(), buf, 1024, disableEcho ? RPP_ECHO_OFF : RPP_ECHO_ON);
    if (ret == NULL) {
        return "";
    }
    return ret;
#else
    struct termios tty;
    tcflag_t oldFlags;

    if (disableEcho) {
        tcgetattr(STDIN_FILENO, &tty);
        oldFlags = tty.c_lflag;
        tty.c_lflag &= ~ECHO;
        (void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    }

    std::cout << prompt.toStdString();
    std::string input;
    std::cin >> input;

    if (disableEcho) {
        std::cout << std::endl;
        tty.c_lflag = oldFlags;
        (void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    }

    return QString::fromStdString(input);
#endif
}

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

    qCDebug(LOG_CLI) << "=== Started ===";
    qCDebug(LOG_CLI) << "OS Version:" << Utils::getOSVersion();

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
        std::cout << "    login [options] [username [password]]" << std::endl;
        std::cout << "        " << "Login with given credentials." << std::endl;
        std::cout << "        " << "If any input has special characters in it, it should be wrapped in single quotes or escaped." << std::endl;
        std::cout << "        " << "If any credentials are not provided, a prompt will be presented." << std::endl;
        std::cout << "    logout [options] [on|off]" << std::endl;
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
        std::cout << "        " << "Configure behavior for the next time during this run that the key limit is reached for WireGuard." << std::endl;
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
        std::cout << "    -2: Use 2-factor authentication.  You will be prompted for the code.  Only for the 'login' operation." << std::endl;
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
            cliArgs.setUsername(getInput("Username: ", false));
        }
        if (cliArgs.password().isEmpty()) {
            cliArgs.setPassword(getInput("Password: ", true));
        }
        if (cliArgs.need2FA()) {
            cliArgs.set2FACode(getInput("2FA code: ", false));
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
    qCDebug(LOG_CLI) << str;
    std::cout << str.toStdString() << std::endl;
}

