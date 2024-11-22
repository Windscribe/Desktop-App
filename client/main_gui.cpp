#include "gui/mainwindow.h"

#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QScreen>
#include <QSocketNotifier>
#include <QWindow>

#include "gui/dpiscalemanager.h"
#include "utils/log/logger.h"
#include "utils/log/paths.h"
#include "utils/utils.h"
#include "utils/extraconfig.h"
#include "version/appversion.h"
#include "engine/openvpnversioncontroller.h"
#include "gui/application/windscribeapplication.h"
#include "gui/application/singleappinstance.h"

#ifdef Q_OS_WIN
    #include "types/global_consts.h"
    #include "utils/crashhandler.h"
    #include "utils/installedantiviruses_win.h"
    #include "utils/winutils.h"
#elif defined (Q_OS_MACOS)
    #include <sys/socket.h>
    #include <unistd.h>
    #include "utils/macutils.h"
#elif defined (Q_OS_LINUX)
    #include <libgen.h>         // dirname
    #include <unistd.h>         // readlink
    #include <linux/limits.h>   // PATH_MAX
    #include <signal.h>
    #include <sys/socket.h>
    #include <sys/types.h>      // gid_t
    #include "utils/linuxutils.h"
#endif

void applyScalingFactor(qreal ldpi, MainWindow &mw);

#if defined (Q_OS_MACOS) || defined (Q_OS_LINUX)
MainWindow *g_MainWindow = NULL;
static int fds[2];

void handler_sigterm(int signum)
{
    if (signum == SIGTERM) {
        qCDebug(LOG_BASIC) << "SIGTERM signal received";

#ifdef Q_OS_LINUX
    WindscribeApplication::instance()->setWasRestartOSFlag();
#endif

        // Can't call Qt functions safely from here, see https://doc.qt.io/qt-6/unix-signals.html
        // Instead, write a byte to a local socket
        char a = 1;
        if (::write(fds[0], &a, sizeof(a)) < 0) {
            qCDebug(LOG_BASIC) << "Could not write to signal socket";
        }
    }
}
#endif

int main(int argc, char *argv[])
{
#if defined (Q_OS_LINUX)
    gid_t gid = LinuxUtils::getWindscribeGid();
    if (gid == -1) {
        qCDebug(LOG_BASIC) << "windscribe group does not exist";
        return -1;
    }
    qCDebug(LOG_BASIC) << "Setting gid to:" << gid;
    if (setgid(gid) < 0) {
        qCDebug(LOG_BASIC) << "Could not set windscribe group";
        return -1;
    }
#endif

#if defined (Q_OS_MACOS) || defined (Q_OS_LINUX)
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds)) {
        qCDebug(LOG_BASIC) << "Could not open signal socket";
    }
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
        const char *path = "";
        if (count != -1) {
            path = dirname(result);
        }
        QStringList pluginsPath;
        pluginsPath << QString::fromStdString(path) + "/plugins";
        QCoreApplication::setLibraryPaths(pluginsPath);
    #endif
#endif

    Q_INIT_RESOURCE(gif);
    Q_INIT_RESOURCE(jpg);
    Q_INIT_RESOURCE(svg);
    Q_INIT_RESOURCE(windscribe);
    #ifdef Q_OS_MACOS
        Q_INIT_RESOURCE(windscribe_mac);
    #endif

    qSetMessagePattern("[{gmt_time} %{time process}] [%{category}]\t %{message}");

#if defined(ENABLE_CRASH_REPORTS)
    Debug::CrashHandler::setModuleName(L"gui");
    Debug::CrashHandler::instance().bindToProcess();
#endif

    qputenv("QT_EVENT_DISPATCHER_CORE_FOUNDATION", "1");

    #ifdef Q_OS_WIN
    // Fixes fuzzy text and graphics on Windows when a display is set to a fractional scaling value (e.g. 150%).
    // Warning: I tried Floor, Round, RoundPreferFloor, and Ceil on Qt 6.2.4 and 6.3.1, and they all produce
    // strange clipping behavior when one minimizes the app, changes the display's scale, then restores the app.
    // Using the Floor setting, at least on Windows 10, appeared to minimize this behavior.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);
    #endif

    WindscribeApplication a(argc, argv);

    // These values are used for QSettings by default
    a.setOrganizationName("Windscribe");
    a.setApplicationName("Windscribe2");

    a.setApplicationDisplayName("Windscribe");

    // Switch to staging if necessary. It should be done at the beginning of main function.
    // Further, in all parts of the program, the staging check is performed by the function AppVersion::instance().isStaging()
    // Works only for guinea pig builds
#ifdef WINDSCRIBE_IS_GUINEA_PIG
    if (ExtraConfig::instance().getIsStaging())
    {
        AppVersion::instance().switchToStaging();
    }
#endif

    // This guard must be created after WindscribeApplication, or its objects will not
    // participate in the main event loop.  It must also be created before the Logger
    // so, if this is the second instance to run, it does not copy the current instance's
    // log_gui to prev_log_gui.
    windscribe::SingleAppInstance appSingleInstGuard;
    if (appSingleInstGuard.isRunning())
    {
        if (!appSingleInstGuard.activatedRunningInstance())
        {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("Windscribe is already running on your computer, but appears to not be responding."));
            msgBox.setInformativeText(QObject::tr("You may need to kill the non-responding Windscribe app or reboot your computer to fix the issue."));
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.exec();
        }

        return 0;
    }

#if defined (Q_OS_MACOS) || defined (Q_OS_LINUX)
    QObject::connect(&appSingleInstGuard, &windscribe::SingleAppInstance::anotherInstanceRunning,
                     &a, &WindscribeApplication::activateFromAnotherInstance);
#endif

    log_utils::Logger::instance().install(log_utils::paths::clientLogLocation(), true);

    qCDebug(LOG_BASIC) << "=== Started ===";
    qCDebug(LOG_BASIC) << "App version:" << AppVersion::instance().fullVersionString();
    qCDebug(LOG_BASIC) << "Platform:" << QGuiApplication::platformName();
    qCDebug(LOG_BASIC) << "OS Version:" << Utils::getOSVersion();
#if defined(Q_OS_LINUX)
    qCDebug(LOG_BASIC) << "Distribution:" << LinuxUtils::getDistroName();
#endif
    qCDebug(LOG_BASIC) << "CPU architecture:" << QSysInfo::currentCpuArchitecture();
    // To aid us in diagnosing possible region-specific issues.
    qCDebug(LOG_BASIC) << "UI languages:" << QLocale::system().uiLanguages();

    ExtraConfig::instance().logExtraConfig();

#ifdef Q_OS_WIN
    InstalledAntiviruses_win::outToLog();
    if (!WinUtils::isOSCompatible()) {
        qCDebug(LOG_BASIC) << "WARNING: OS version is not fully compatible.  Windows 10 build"
                           << kMinWindowsBuildNumber << "or newer is required for full functionality.";
    }
#endif

    if (!QFileInfo::exists(OpenVpnVersionController::instance().getOpenVpnFilePath())) {
        qCDebug(LOG_BASIC) << "OpenVPN executable not found";
        return 0;
    }

#if defined Q_OS_MACOS
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

#if defined (Q_OS_MACOS) || defined (Q_OS_LINUX)
    g_MainWindow = &w;
    w.setSigTermHandler(fds[1]);
#endif
    w.showAfterLaunch();

    int ret = a.exec();
#if defined (Q_OS_MACOS) || defined (Q_OS_LINUX)
    g_MainWindow = nullptr;
#endif
    appSingleInstGuard.release();

    return ret;
}

