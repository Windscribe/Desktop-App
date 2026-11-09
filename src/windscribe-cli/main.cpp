#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QObject>
#include <QProcess>
#include <iostream>

#include "backendcommander.h"
#include "cliarguments.h"
#include "help.h"
#include "strings.h"
#include "utils/log/logger.h"
#include "utils/log/paths.h"
#include "utils/systemwideapplock.h"
#include "utils/utils.h"
#include "version/ws_version.h"

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

// Errors go to stderr, everything else to stdout.
void logAndCout(const QString &str, bool isError = false);

int main(int argc, char *argv[])
{
    // clear Qt plugin library paths for release build
#ifndef QT_DEBUG
    QCoreApplication::setLibraryPaths(QStringList());
#endif

    QCoreApplication::setApplicationName(WS_SETTINGS_APP);
    QCoreApplication::setOrganizationName(WS_SETTINGS_ORG);

    QCoreApplication a(argc, argv);

    CliArguments cliArgs;
    cliArgs.processArguments();

    // Help, version, and argument errors don't talk to the app, so they work while another CLI instance is running.
    // They don't log either: installing the logger rotates the log file another instance may be writing.
    if (cliArgs.cliCommand() == CLI_COMMAND_NONE || cliArgs.cliCommand() == CLI_COMMAND_HELP) {
        std::cout << (cliArgs.helpTopic().isEmpty() ? Help::overview() : Help::command(cliArgs.helpTopic())).toStdString();
        return 0;
    }

    if (cliArgs.cliCommand() == CLI_COMMAND_VERSION) {
        std::cout << "windscribe-cli " << WS_VERSION << std::endl;
        return 0;
    }

    if (cliArgs.cliCommand() == CLI_COMMAND_INVALID) {
        std::cerr << "Error: " << cliArgs.error().toStdString() << std::endl;
        if (cliArgs.helpTopic().isEmpty()) {
            std::cerr << "Run \"windscribe-cli --help\" for a list of commands." << std::endl;
        } else {
            std::cerr << std::endl << Help::usage(cliArgs.helpTopic()).toStdString();
            std::cerr << std::endl << "Run \"windscribe-cli " << cliArgs.helpTopic().toStdString() << " --help\" for more information." << std::endl;
        }
        return 1;
    }

#ifdef CLI_ONLY
    if (cliArgs.cliCommand() == CLI_COMMAND_RELOAD_CONFIG) {
        std::cout << "Preferences are applied automatically as soon as the conf file is saved; reloading is no longer necessary." << std::endl;
        return 0;
    }
#endif

    if (Utils::isCLIRunning(1)) {
        std::cerr << "Windscribe CLI is already running" << std::endl;
        return 1;
    }

    log_utils::Logger::instance().install(log_utils::paths::cliLogLocation(), false);

    qCInfo(LOG_CLI) << "=== Started ===";
    qCInfo(LOG_CLI) << "OS Version:" << Utils::getOSVersion();

    // Before anything is started or prompted for: another user's app holds the helper, and ours would only
    // quit. Read-only, so it cannot get in the way of an app that is starting up.
    if (SystemWideAppLock::holder() == SystemWideAppLock::Holder::kOtherUser) {
        logAndCout(appRunningInAnotherSessionString(), true);
        return 1;
    }

    BackendCommander *backendCommander = new BackendCommander(cliArgs);

    QObject::connect(backendCommander, &BackendCommander::finished, [&](int returnCode, const QString &msg) {
        logAndCout(msg, returnCode != 0);
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
            logAndCout("Error: this application should not run as root.", true);
            return -1;
        }

        int err = system("systemctl --user daemon-reload && systemctl --user enable windscribe && systemctl --user start windscribe");
        if (err != 0) {
            logAndCout("Could not start application", true);
            return err;
        }
        backendCommander->initAndSend();
#else
        // On Linux the CLI is run through the /usr/bin/windscribe-cli symlink, so resolve it to find the app beside the real binary.
        const QString workingDir = QFileInfo(QCoreApplication::applicationFilePath()).canonicalPath();
#ifdef Q_OS_WIN
        QString appPath = workingDir + "/Windscribe.exe";
#else
        QString appPath = workingDir + "/Windscribe";
#endif

#ifdef Q_OS_WIN
        QProcess::startDetached(appPath, QStringList(), workingDir);
        backendCommander->initAndSend();
#else
        // use non-static start detached to prevent GUI output from polluting cli
        QProcess process;
#ifdef Q_OS_LINUX
        process.setProgram("systemd-run");
        process.setArguments({"--user", "--collect", "--", appPath});
#else
        process.setProgram(appPath);
#endif
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

void logAndCout(const QString &str, bool isError)
{
    if (str.isEmpty()) {
        return;
    }
    qCInfo(LOG_CLI) << str;
    (isError ? std::cerr : std::cout) << str.toStdString() << std::endl;
}

