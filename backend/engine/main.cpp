#include "application/windscribeapplication.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/openvpnversioncontroller.h"
#include "engine/curlinitcontroller.h"
#include "engine/engine.h"
#include "engineserver.h"
#include "qconsolelistener.h"
#include "version/appversion.h"
#include "utils/executable_signature/executable_signature.h"

#ifdef Q_OS_WIN
    #include "engine/taputils/tapinstall_win.h"
    #include "utils/installedantiviruses_win.h"
#elif defined Q_OS_MAC
    #include "utils/macutils.h"
    #include "engine/helper/installhelper_mac.h"
#endif

#ifdef Q_OS_MAC
    void handler_sigterm(int signum)
    {
        /*Q_UNUSED(signum);
        qCDebug(LOG_BASIC) << "SIGTERM signal received";
        if (g_MainWindow)
        {
            g_MainWindow->doClose(NULL, true);
        }*/
        exit(0);
    }
#else
    void handler_sigterm(int signum)
    {
        qCDebug(LOG_BASIC) << "SIGTERM signal receidved";
    }

LONG WINAPI crashHandler(EXCEPTION_POINTERS * /*ExceptionInfo*/)
{
    qCDebug(LOG_BASIC) << "SIGTERM signal received";
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif


int main(int argc, char *argv[])
{

#ifdef Q_OS_WIN
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        return hr;
    }
#endif

    //::SetUnhandledExceptionFilter(crashHandler);
    //signal(SIGINT, handler_sigterm);

    qputenv("QT_EVENT_DISPATCHER_CORE_FOUNDATION", "1");

    WindscribeApplication a(argc, argv);
    qSetMessagePattern("[{gmt_time} %{time process}] [%{category}]\t %{message}");

    WindscribeApplication::setApplicationName("Windscribe");
    WindscribeApplication::setOrganizationName("Windscribe");

    CurlInitController curlInitController;
    curlInitController.init();


/*
#ifdef Q_OS_MAC
    signal(SIGTERM, handler_sigterm);
#endif
*/

    Logger::instance().install("engine", true);
    qCDebug(LOG_BASIC) << "App start time:" << QDateTime::currentDateTime().toString();
    qCDebug(LOG_BASIC) << "OS Version:" << Utils::getOSVersion();

#ifdef Q_OS_WIN
    qCDebug(LOG_BASIC) << "Windscribe Windows version: " << AppVersion::instance().getFullString();
#elif defined Q_OS_MAC
    qCDebug(LOG_BASIC) << "Windscribe Mac version: " << AppVersion::instance().getFullString();
#endif

    if (!ExecutableSignature::isParentProcessGui())
    {
        qCDebug(LOG_BASIC) << "abd444";  // a meaningless message that is useful for us, but not for hackers
        return 0;
    }

#ifdef Q_OS_WIN
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
