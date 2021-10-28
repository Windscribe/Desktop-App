#include "application/windscribeapplication.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/openvpnversioncontroller.h"
#include "engine/engine.h"
#include "engineserver.h"
#include "qconsolelistener.h"
#include "version/appversion.h"
#include "utils/executable_signature/executable_signature.h"

#ifdef Q_OS_WIN
    #include "engine/taputils/tapinstall_win.h"
    #include "utils/installedantiviruses_win.h"
    #include "utils/crashhandler.h"
#elif defined Q_OS_MAC
    #include "utils/macutils.h"
    #include "engine/helper/installhelper_mac.h"
#endif


#if defined (Q_OS_MAC) || defined (Q_OS_LINUX)
    void handler_sigterm(int signum)
    {
        if (signum == SIGTERM)
        {
            qCDebug(LOG_BASIC) << "SIGTERM signal received";
        }
    }
#endif

int main(int argc, char *argv[])
{

#if defined (Q_OS_MAC) || defined (Q_OS_LINUX)
    signal(SIGTERM, handler_sigterm);
#endif

#ifdef Q_OS_WIN
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        return hr;
    }
#endif
#if defined(ENABLE_CRASH_REPORTS)
    Debug::CrashHandler::setModuleName(L"engine");
    Debug::CrashHandler::instance().bindToProcess();
#endif

    // clear Qt plugin library paths for release build
#ifndef QT_DEBUG
    QCoreApplication::setLibraryPaths(QStringList());
#endif

    qputenv("QT_EVENT_DISPATCHER_CORE_FOUNDATION", "1");

    WindscribeApplication a(argc, argv);
    qSetMessagePattern("[{gmt_time} %{time process}] [%{category}]\t %{message}");

    // These values are used for QSettings by default
    WindscribeApplication::setOrganizationName("Windscribe");
    WindscribeApplication::setApplicationName("Windscribe2");

    Logger::instance().install("engine", true, a.isRecoveryMode());
    if (a.isRecoveryMode()) qCDebug(LOG_BASIC) << "Engine started in recovery mode.";
    qCDebug(LOG_BASIC) << "App start time:" << QDateTime::currentDateTime().toString();
    qCDebug(LOG_BASIC) << "OS Version:" << Utils::getOSVersion();

#ifdef Q_OS_WIN
    qCDebug(LOG_BASIC) << "Windscribe Windows version: " << AppVersion::instance().fullVersionString();
#elif defined Q_OS_MAC
    qCDebug(LOG_BASIC) << "Windscribe Mac version: " << AppVersion::instance().fullVersionString();
#endif

    if (!ExecutableSignature::isParentProcessGui())
    {
        qCDebug(LOG_BASIC) << "abd444";  // a meaningless message that is useful for us, but not for hackers
        return 0;
    }

#ifdef Q_OS_WIN
    // Make sure that on shutdown, Windows doesn't send a shutdown signal to the engine prior to the
    // GUI. Since the engine is a console app and has no windows, it cannot intercept any shutdown
    // messages and postpone a closure. We allow the GUI to perform cleanup by prioritizing shutdown
    // of the engine with the lowest possible application reserved value, 0x100 (the default value
    // is 0x280).
    SetProcessShutdownParameters(0x100, 0);

    InstalledAntiviruses_win::outToLog();
#endif

    QStringList strs = OpenVpnVersionController::instance().getAvailableOpenVpnVersions();
    if (strs.count() > 0)
    {
        qCDebug(LOG_BASIC) << "Detected OpenVPN versions:" << strs;
        qCDebug(LOG_BASIC) << "Selected OpenVPN version:" << OpenVpnVersionController::instance().getSelectedOpenVpnVersion();
    }
    else
    {
        qCDebug(LOG_BASIC) << "OpenVPN executables not found";
        return 0;
    }

#ifdef Q_OS_WIN
    QVector<TapInstall_win::TAP_TYPE> tapAdapters = TapInstall_win::detectInstalledTapDriver(true);
    if (tapAdapters.isEmpty())
    {
        qCDebug(LOG_BASIC) << "Can't detect installed TAP-adapter";
    }
#endif

    EngineServer *engineServer = new EngineServer(&a);
    QObject::connect(engineServer, SIGNAL(finished()), &a, SLOT(quit()));
    QTimer::singleShot(0, engineServer, SLOT(run()));

    int ret = a.exec();

    qCDebug(LOG_BASIC) << "Closed";
    return ret;
}
