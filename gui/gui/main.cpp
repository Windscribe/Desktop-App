#include "mainwindow.h"
#include <QApplication>
#include <QScreen>
#include <QWindow>
#include <QMessageBox>
#include "dpiscalemanager.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "application/windscribeapplication.h"
#include "graphicresources/imageresourcessvg.h"

#ifdef Q_OS_WIN
    #include "application/preventmultipleinstances_win.h"
    #include "utils/scaleutils_win.h"
    #include "utils/crashhandler.h"
#elif defined (Q_OS_MACOS)
    #include "utils/macutils.h"
#elif defined (Q_OS_LINUX)
    #include <libgen.h>         // dirname
    #include <unistd.h>         // readlink
    #include <linux/limits.h>   // PATH_MAX
#endif

void applyScalingFactor(qreal ldpi, MainWindow &mw);

#ifdef Q_OS_MAC
MainWindow *g_MainWindow = NULL;
    void handler_sigterm(int signum)
    {
        Q_UNUSED(signum);
        qCDebug(LOG_BASIC) << "SIGTERM signal received";
        if (g_MainWindow)
        {
            g_MainWindow->doClose(NULL, true);
        }
        exit(0);
    }
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // prevent multiple instances of process for Windows
    PreventMultipleInstances_win multipleInstances;
    if (!multipleInstances.lock())
    {
        return 0;
    }
#endif

#ifdef Q_OS_MAC
    signal(SIGTERM, handler_sigterm);
#endif


    // set Qt plugin library paths for release build
#ifndef QT_DEBUG
    #ifdef Q_OS_WIN
        // For Windows an empty list means searching plugins in the executable folder
        QCoreApplication::setLibraryPaths(QStringList());
    #elif defined (Q_OS_MACOS)
        QStringList pluginsPath;
        pluginsPath << MacUtils::getBundlePath() + "/Contents/PlugIns";
        QCoreApplication::setLibraryPaths(pluginsPath);
    #elif defined (Q_OS_LINUX)
        //todo move to LinuxUtils
        char result[PATH_MAX] = {};
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        const char *path;
        if (count != -1) {
            path = dirname(result);
        }
        QStringList pluginsPath;
        pluginsPath << QString::fromStdString(path) + "/plugins";
        QCoreApplication::setLibraryPaths(pluginsPath);
    #endif
#endif

    qSetMessagePattern("[{gmt_time} %{time process}] [%{category}]\t %{message}");

#if defined(ENABLE_CRASH_REPORTS)
    Debug::CrashHandler::setModuleName(L"gui");
    Debug::CrashHandler::instance().bindToProcess();
#endif

#ifdef Q_OS_MAC
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#elif defined (Q_OS_LINUX)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    WindscribeApplication a(argc, argv);

    bool guiInstanceAlreadyRunning = Utils::giveFocusToGui();
    if (guiInstanceAlreadyRunning)
    {
        qCDebug(LOG_BASIC) << "GUI appears to be running -- quitting";
        return 0;
    }

    // These values are used for QSettings by default
    a.setOrganizationName("Windscribe");
    a.setApplicationName("Windscribe2");

    a.setApplicationDisplayName("Windscribe");


    Logger::instance().install("gui", true);

    qCDebug(LOG_BASIC) << "App start time:" << QDateTime::currentDateTime().toString();
    qCDebug(LOG_BASIC) << "OS Version:" << Utils::getOSVersion();

#if defined Q_OS_MAC
    if (!MacUtils::isOsVersion10_11_or_greater())
    {
        QMessageBox msgBox;
        msgBox.setText( QObject::tr("Windscribe is not compatible with your version of OSX. Please upgrade to 10.11+") );
        msgBox.setIcon( QMessageBox::Information );
        msgBox.exec();
        return 0;
    }

    if (!MacUtils::verifyAppBundleIntegrity())
    {
        QMessageBox msgBox;
        msgBox.setText( QObject::tr("One or more files in the Windscribe application bundle have been suspiciously modified. Please re-install Windscribe.") );
        msgBox.setIcon( QMessageBox::Critical );
        msgBox.exec();
        return 0;
    }
#endif
    a.setStyle("fusion");

    DpiScaleManager::instance();    // init dpi scale manager

    MainWindow w;
#ifdef Q_OS_MAC
    g_MainWindow = &w;
#endif
    w.showAfterLaunch();

#ifdef Q_OS_WIN
    multipleInstances.unlock();
#endif
    int ret = a.exec();
#ifdef Q_OS_MAC
    g_MainWindow = nullptr;
#endif
    ImageResourcesSvg::instance().finishGracefully();
    return ret;
}

