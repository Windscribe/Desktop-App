#include "launchonstartup.h"

#include <QtGlobal>

#ifdef Q_OS_WIN
    #include "launchonstartup_win.h"
#else
    #include "launchonstartup_mac.h"
#endif

void LaunchOnStartup::setLaunchOnStartup(bool enable)
{
#ifdef Q_OS_WIN
    LaunchOnStartup_win::setLaunchOnStartup(enable);
#elif defined Q_OS_MAC
    LaunchOnStartup_mac::setLaunchOnStartup(enable);
#elif defined Q_OS_LINUX
        //todo linux
        //Q_ASSERT(false);
    Q_UNUSED(enable);
#endif
}

