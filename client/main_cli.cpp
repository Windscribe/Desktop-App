#include <QCoreApplication>
#include <QFileInfo>

#include "application/singleappinstance.h"
#include "application/windscribeapplication.h"
#include "cli/mainservice.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/extraconfig.h"
#include "version/appversion.h"
#include "engine/openvpnversioncontroller.h"

#if defined (Q_OS_LINUX)
    #include <libgen.h>         // dirname
    #include <unistd.h>         // readlink
    #include <linux/limits.h>   // PATH_MAX
    #include <signal.h>
    #include <sys/socket.h>
    #include <sys/types.h>      // gid_t
    #include "utils/linuxutils.h"
#endif

static int fds[2];

#if defined (Q_OS_LINUX)
void handler_sigterm(int signum)
{
    if (signum == SIGTERM) {
        qCDebug(LOG_BASIC) << "SIGTERM signal received";

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

#if defined (Q_OS_LINUX)
    signal(SIGTERM, handler_sigterm);

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds)) {
        qCDebug(LOG_BASIC) << "Could not create socket pair";
        return -1;
    }

    // set Qt plugin library paths for release build
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

    qSetMessagePattern("[{gmt_time} %{time process}] [%{category}]\t %{message}");

    qputenv("QT_EVENT_DISPATCHER_CORE_FOUNDATION", "1");

    // Unlike the GUI application, the guard does not participate in the event loop anyway,
    // so it can be started early.
    windscribe::SingleAppInstance appSingleInstGuard;
    if (appSingleInstGuard.isRunning()) {
        qCDebug(LOG_BASIC) << "Windscribe already running";
        return 0;
    }

    WindscribeApplication a(argc, argv);
    a.setSigTermHandler(fds[1]);

    // These values are used for QSettings by default
    a.setOrganizationName("Windscribe");
    a.setApplicationName("Windscribe2");

    // Switch to staging if necessary. It should be done at the beginning of main function.
    // Further, in all parts of the program, the staging check is performed by the function AppVersion::instance().isStaging()
    // Works only for guinea pig builds
#ifdef WINDSCRIBE_IS_GUINEA_PIG
    if (ExtraConfig::instance().getIsStaging()) {
        AppVersion::instance().switchToStaging();
    }
#endif

    Logger::instance().install("cli", true, false);

    qCDebug(LOG_BASIC) << "App start time:" << QDateTime::currentDateTime().toString();
    qCDebug(LOG_BASIC) << "App version:" << AppVersion::instance().fullVersionString();
    qCDebug(LOG_BASIC) << "OS Version:" << Utils::getOSVersion();
    qCDebug(LOG_BASIC) << "Platform: CLI";
#if defined(Q_OS_LINUX)
    qCDebug(LOG_BASIC) << "Distribution:" << LinuxUtils::getDistroName();
#endif
    qCDebug(LOG_BASIC) << "CPU architecture:" << QSysInfo::currentCpuArchitecture();
    // To aid us in diagnosing possible region-specific issues.
    qCDebug(LOG_BASIC) << "UI languages:" << QLocale::system().uiLanguages();

    ExtraConfig::instance().logExtraConfig();

    // We do this here so that the service constructor happeens after log initialization & changing gid
    MainService *service = new MainService();
    a.setService(service);

    if (!QFileInfo::exists(OpenVpnVersionController::instance().getOpenVpnFilePath())) {
        qCDebug(LOG_BASIC) << "OpenVPN executable not found";
        return 0;
    }

    int ret = a.exec();
    qCDebug(LOG_BASIC) << "Releasing lock";
    appSingleInstGuard.release();
    return ret;
}
