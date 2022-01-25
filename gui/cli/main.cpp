#include <QCoreApplication>

#include <QObject>
#include <QProcess>
#include <QDateTime>
#include <iostream>
//#include "backendcommander.h"
#include "cliapplication.h"
#include "utils/logger.h"
#include "utils/utils.h"

#ifdef Q_OS_WIN
    #include "Windows.h"
#else
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

    CliApplication a(argc, argv);
    if (a.cliCommand() == CLI_COMMAND_NONE)
    {
        qCDebug(LOG_BASIC) << "CLI args fail: Couldn't determine appropriate command from arguments";
        QString output = QObject::tr("There appears to be an issue with the provided arguments. Try 'windscribe-cli ?' to see available options");
        std::cout << output.toStdString() << std::endl;
        return 0;
    }
    else if (a.cliCommand() == CLI_COMMAND_HELP)
    {
        qCDebug(LOG_BASIC) << "Printing help menu";
        std::cout << "windscribe-cli.exe supports the following commands:" << std::endl;
        std::cout << "connect                   - Connects to last connected location. If no last location, connect to best location." << std::endl;
        std::cout << "connect best              - Connects to best location                  " << std::endl;
        std::cout << "connect \"RegionName\"      - Connects to a random datacenter in the region" << std::endl;
        std::cout << "connect \"CityName\"        - Connects to a random datacenter in the city  " << std::endl;
        std::cout << "connect \"Nickname\"        - Connects to datacenter with same nickname  " << std::endl;
        std::cout << "connect \"ISO CountryCode\" - Connects to a random data center in the country" << std::endl;
        std::cout << "disconnect                - Disconnects from current datacenter              " << std::endl;
        std::cout << "firewall on|off           - Turn firewall ON/OFF                     " << std::endl;
        std::cout << "locations                 - View a list of available locations" << std::endl;
        return 0;
    }

    /*BackendCommander *backendCommander = new BackendCommander(a.cliCommand(), a.location());

    bool alreadyRun = false;
    QObject::connect(backendCommander, &BackendCommander::finished, [&](const QString &msg){
        if (!alreadyRun)
        {
            alreadyRun = true;
            logAndCout(msg);
            backendCommander->closeBackendConnection();
            delete backendCommander;
            a.quit();
        }
    });

    QObject::connect(backendCommander, &BackendCommander::report, [&](const QString &msg){
        logAndCout(msg);
    });*/

    if (Utils::isGuiAlreadyRunning())
    {
        logAndCout(QCoreApplication::tr("GUI detected -- attempting Engine connect"));
        //backendCommander->initAndSend();
    }
    else
    {
        logAndCout(QCoreApplication::tr("No GUI instance detected, starting one now..."));
/*
        // GUI pathing
#ifdef Q_OS_WIN
        QString guiPath = QCoreApplication::applicationDirPath() + "/Windscribe.exe";
#elif defined Q_OS_MAC
        QString guiPath = QCoreApplication::applicationDirPath() + "/Windscribe";
#endif
        QString workingDir = QCoreApplication::applicationDirPath();

#ifdef QT_DEBUG
    #ifdef Q_OS_WIN
        // windows
        QString debugFolderPath = "C:/Work/client-desktop-gui/build-WindscribeGui-Qt5_12_4_MSVC15_2017-Debug/debug";
        guiPath = debugFolderPath  + "/Windscribe.exe";
    #endif

    #ifdef Q_OS_MAC
        // mac
        QString debugFolderPath = "/Users/eguloien/work/client-desktop-gui/build-WindscribeGui-Clang_C_x86_64_Qt5_12_7-Debug/Windscribe.app/Contents/MacOS";
        guiPath = debugFolderPath  + "/Windscribe";
        workingDir = debugFolderPath;
    #endif
#endif

#ifdef Q_OS_WIN
        HANDLE guiInitEvent = CreateEvent(NULL, TRUE, FALSE, TEXT("WindscribeGuiStarted"));
        if (guiInitEvent == NULL)
        {
            logAndCout(QCoreApplication::tr("Aborting: Failed to create GuiStarted object"));
            delete backendCommander;
            return 0;
        }

        QProcess::startDetached(guiPath, QStringList(), workingDir);

        if (WaitForSingleObject(guiInitEvent, 10000) == WAIT_OBJECT_0)
        {
            backendCommander->initAndSend();
            CloseHandle(guiInitEvent);
        }
        else
        {
            logAndCout(QCoreApplication::tr("Aborting: Gui did not start in time"));
            CloseHandle(guiInitEvent);
            delete backendCommander;
            return 0;
        }
#else
        const std::string guiStartedStr = "WindscribeGuiStarted";
        sem_unlink(guiStartedStr.c_str());
        sem_t *sem = sem_open(guiStartedStr.c_str(), O_CREAT | O_EXCL, 0666, 0);
        if (sem == SEM_FAILED)
        {
            fprintf(stderr, "sem_open() failed.  errno:%d\n", errno);
            logAndCout(QCoreApplication::tr("Aborting: Failed to create WindscribeGuiStarted semaphore"));
            return 0;
        }

        // use non-static start detached to prevent GUI output from polluting cli
        QProcess process;
        process.setProgram(guiPath);
        process.setWorkingDirectory(workingDir);
        process.setStandardOutputFile(QProcess::nullDevice());
        process.setStandardErrorFile(QProcess::nullDevice());
        qint64 pid;
        process.startDetached(&pid);

        // wait for GUI signal for 10s
        int i = 10;
        while (i > 0)
        {
            if (sem_trywait(sem) == 0)
            {
                logAndCout(QCoreApplication::tr("Received GUI signal -- Connecting to Engine"));
                backendCommander->initAndSend();
                break;
            }

            logAndCout("Waiting...");
            sleep(1);
            i--;
        }

        if (i == 0) // failed
        {
            logAndCout(QCoreApplication::tr("Aborting: Gui did not start in time"));
            return 0;
        }

        sem_close(sem);
        sem_unlink(guiStartedStr.c_str());
#endif*/
    }

    return a.exec();
}

void logAndCout(const QString &str)
{
    qCDebug(LOG_BASIC) << str;
    std::cout << str.toStdString() << std::endl;
}

