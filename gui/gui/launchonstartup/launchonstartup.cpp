#include "launchonstartup.h"

#include <QtGlobal>

#ifdef Q_OS_WIN
    #include "launchonstartup_win.h"
#elif defined(Q_OS_MAC)
    #include "launchonstartup_mac.h"
#elif defined (Q_OS_LINUX)
    #include "launchonstartup_linux.h"
#endif

void LaunchOnStartup::setLaunchOnStartup(bool enable)
{
#ifdef Q_OS_WIN
    LaunchOnStartup_win::setLaunchOnStartup(enable);
#elif defined Q_OS_MAC
    LaunchOnStartup_mac::setLaunchOnStartup(enable);
#elif defined Q_OS_LINUX
    LaunchOnStartup_linux::setLaunchOnStartup(enable);
#endif
}

