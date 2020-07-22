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
#else
    LaunchOnStartup_mac::setLaunchOnStartup(enable);
#endif
}

