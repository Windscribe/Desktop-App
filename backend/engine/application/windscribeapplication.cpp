#include "windscribeapplication.h"
#include "utils/logger.h"
#include <QAbstractEventDispatcher>

#ifdef Q_OS_MAC

#include <objc/objc.h>
#include <objc/message.h>

#endif

WindscribeApplication::WindscribeApplication(int &argc, char **argv) : QCoreApplication(argc, argv)
  , bNeedAskClose_(false)
  , bWasRestartOS_(false)
  , bIsRecoveryMode_(false)
{
#ifdef Q_OS_WIN
    QAbstractEventDispatcher::instance()->installNativeEventFilter(&windowsNativeEventFilter_);
#endif

#ifdef Q_OS_MAC
    connect(&exitHanlderMac_, SIGNAL(shouldTerminate()), SIGNAL(shouldTerminate_mac()));
#endif

    // check for recovery flag
    std::vector<std::string> args(argv, argv+argc);
    for (const std::string &arg : args)
    {
        if (arg == "--recovery")
        {
            bIsRecoveryMode_ = true;
            break;
        }
    }
}

bool WindscribeApplication::isExitWithRestart()
{
#ifdef Q_OS_MAC
    return exitHanlderMac_.isExitWithRestart();
#else
    return bWasRestartOS_;
#endif
}

