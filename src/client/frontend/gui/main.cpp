#include "mainwindow.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QMessageBox>
#include <QScreen>
#include <QSocketNotifier>
#include <QTimer>
#include <QWindow>

#include "dpiscalemanager.h"
#include "utils/log/logger.h"
#include "utils/log/paths.h"
#include "utils/utils.h"
#include "utils/extraconfig.h"
#include "version/appversion.h"
#include "utils/openvpnversioncontroller.h"
#include "application/windscribeapplication.h"
#include "application/singleappinstance.h"

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
    #include "utils/linuxutils.h"
    #include "utils/helperconnector_linux.h"
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
            qCWarning(LOG_BASIC) << "Could not write to signal socket";
        }
    }
}
#endif

#ifdef Q_OS_MACOS
constexpr int kDisplaySettleMs = 1000;
constexpr int kDisplayWaitTimeoutMs = 60000;

// True only when the display configuration is fully realized: the OS reports an active display, Qt
// has a primary screen, and every QScreen has a live platform handle and real geometry.
static bool displayConfigurationUsable()
{
    if (!MacUtils::hasActiveDisplay()) {
        return false;
    }
    const QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.isEmpty() || QGuiApplication::primaryScreen() == nullptr) {
        return false;
    }
    for (const QScreen *screen : screens) {
        if (screen->handle() == nullptr || screen->geometry().isEmpty()) {
            return false;
        }
    }
    return true;
}

// When the app is autostarted at login (by the SMLoginItem launcher), the window server is still
// publishing displays. The first native window we realize ends up deep in
// QCocoaWindow::createNSWindow, which resolves the window's QCocoaScreen from QGuiApplication::screens();
// if a display reconfiguration tears those screens down mid-construction (Qt processes the AppKit
// screen-change events while the backend/engine init pumps the event loop), createNSWindow
// dereferences a null QCocoaScreen and crashes. The state is transient, so an instantaneous "is there
// a screen" check is not enough: it can pass and then a teardown can still land during construction.
// Instead, wait until the configuration is usable AND has been quiescent (no screen add/remove/primary
// change) for kDisplaySettleMs, i.e. until the login-time reconfiguration storm is over. Only autostart
// launches pay this wait; a manual launch happens long after the window server has settled, so it takes
// the instant fast path. Returns false if no stable configuration appears within the backstop window,
// in which case the caller should exit cleanly rather than crash.
static bool waitForUsableDisplay(bool launchedOnStartup)
{
    if (!launchedOnStartup && displayConfigurationUsable()) {
        return true;
    }

    qCInfo(LOG_BASIC) << "Waiting for a stable display configuration before building the UI";
    QElapsedTimer elapsed;
    elapsed.start();

    QEventLoop loop;

    QTimer backstop;
    backstop.setSingleShot(true);
    QObject::connect(&backstop, &QTimer::timeout, &loop, &QEventLoop::quit);

    QTimer settle;
    settle.setSingleShot(true);
    QObject::connect(&settle, &QTimer::timeout, &loop, [&loop]() {
        if (displayConfigurationUsable()) {
            loop.quit();
        }
    });

    // Any configuration change restarts the quiet period, so we only proceed once the storm subsides.
    auto onConfigChanged = [&settle](QScreen *) {
        settle.start(kDisplaySettleMs);
    };
    QObject::connect(qApp, &QGuiApplication::screenAdded, &loop, onConfigChanged);
    QObject::connect(qApp, &QGuiApplication::screenRemoved, &loop, onConfigChanged);
    QObject::connect(qApp, &QGuiApplication::primaryScreenChanged, &loop, onConfigChanged);

    backstop.start(kDisplayWaitTimeoutMs);
    settle.start(kDisplaySettleMs);
    loop.exec();

    if (!displayConfigurationUsable()) {
        qCWarning(LOG_BASIC) << "No stable display configuration after" << elapsed.elapsed() << "ms; exiting";
        return false;
    }
    qCInfo(LOG_BASIC) << "Display configuration stable after" << elapsed.elapsed() << "ms; continuing startup";
    return true;
}
#endif

int main(int argc, char *argv[])
{
#if defined (Q_OS_MACOS) || defined (Q_OS_LINUX)
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds)) {
        qCCritical(LOG_BASIC) << "Could not open signal socket";
    }
    signal(SIGTERM, handler_sigterm);
#endif

    // set Qt plugin library paths for non-dev builds
#ifndef WINDSCRIBE_DEV_MODE
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

#if defined(Q_OS_LINUX)
#ifndef WINDSCRIBE_DEV_MODE
    // SGID 'windscribe' GUI: neutralize env that could load attacker code, then drop the group.
    // Must precede the gid drop: some loaders (e.g. Mesa) re-honor these once egid == gid.
    // GL is disabled (pure Widgets, no GL context); QT_QPA_PLATFORM_PLUGIN_PATH is read separately
    // from setLibraryPaths above, so unset it to keep the platform plugin in the bundled dir.
    if (!qputenv("QT_XCB_GL_INTEGRATION", "none") || !qunsetenv("QT_QPA_PLATFORM_PLUGIN_PATH")) {
        qCCritical(LOG_BASIC) << "Could not neutralize environment";
        return -1;
    }
#endif

    // Open the helper connection now, on a background thread, while we still hold the 'windscribe'
    // group (from the binary's setgid bit). The engine adopts the connected fd later. This must run
    // before the group drop below so the thread inherits the group; the connect itself is async.
    HelperConnector::startConnect();

    // Permanently drop 'windscribe' from this (main/GUI) thread: libdbus refuses the session bus
    // (tray, notifications) whenever the saved gid differs from the real gid, and the first bus
    // access happens during MainWindow construction.
    if (!LinuxUtils::dropHelperGroup()) {
        return -1;
    }
#endif

    Q_INIT_RESOURCE(images);
    Q_INIT_RESOURCE(sounds);
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
#if defined(Q_OS_LINUX)
    qputenv("QT_WAYLAND_DECORATION", "bradient");
#endif

    #ifdef Q_OS_WIN
    // Fixes fuzzy text and graphics on Windows when a display is set to a fractional scaling value (e.g. 150%).
    // Warning: I tried Floor, Round, RoundPreferFloor, and Ceil on Qt 6.2.4 and 6.3.1, and they all produce
    // strange clipping behavior when one minimizes the app, changes the display's scale, then restores the app.
    // Using the Floor setting, at least on Windows 10, appeared to minimize this behavior.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);
    #endif

    WindscribeApplication a(argc, argv);

    // These values are used for QSettings by default
    a.setOrganizationName(WS_SETTINGS_ORG);
    a.setApplicationName(WS_SETTINGS_APP);

    a.setApplicationDisplayName(WS_PRODUCT_NAME);

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

    qCInfo(LOG_BASIC) << "=== Started ===";
    qCInfo(LOG_BASIC) << "App version:" << AppVersion::instance().fullVersionString();
    qCInfo(LOG_BASIC) << "Platform:" << QGuiApplication::platformName();
    qCInfo(LOG_BASIC) << "OS Version:" << Utils::getOSVersion();
#if defined(Q_OS_LINUX)
    qCInfo(LOG_BASIC) << "Distribution:" << LinuxUtils::getDistroName();
#endif
    qCInfo(LOG_BASIC) << "CPU architecture:" << QSysInfo::currentCpuArchitecture();
    // To aid us in diagnosing possible region-specific issues.
    qCInfo(LOG_BASIC) << "UI languages:" << QLocale::system().uiLanguages();

    ExtraConfig::instance().logExtraConfig();

#ifdef Q_OS_WIN
    InstalledAntiviruses_win::outToLog();
    if (!WinUtils::isOSCompatible()) {
        qCWarning(LOG_BASIC) << "WARNING: OS version is not fully compatible.  Windows 10 build"
                           << kMinWindowsBuildNumber << "or newer is required for full functionality.";
    }
#endif

#if !defined(WINDSCRIBE_DEV_MODE)
    if (!QFileInfo::exists(OpenVpnVersionController::instance().getOpenVpnFilePath())) {
        qCCritical(LOG_BASIC) << "OpenVPN executable not found";
        return 0;
    }
#endif

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

#ifdef Q_OS_MACOS
    // Mirrors the flags MainWindow::wasLaunchedOnStartup() recognizes; checked here without a
    // QCommandLineParser since MainWindow does not exist yet and process() can exit on unknown args.
    const QStringList appArgs = a.arguments();
    const bool launchedOnStartup = appArgs.contains(QStringLiteral("--autostart"))
        || appArgs.contains(QStringLiteral("-autostart"))
        || appArgs.contains(QStringLiteral("--os_restart"))
        || appArgs.contains(QStringLiteral("-os_restart"));
    if (!waitForUsableDisplay(launchedOnStartup)) {
        return 0;
    }
#endif

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

