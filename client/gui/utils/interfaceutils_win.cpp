#include "interfaceutils.h"
#include "utils/winutils.h"
#include "utils/logger.h"

namespace InterfaceUtils
{

bool isDarkMode()
{
    // for Windows 7 or 8 always use dark icons
    if (WinUtils::isWindowsVISTAor7or8())
    {
        return false;
    }

    int dwordValue = 0;
    const QString subKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    if (!WinUtils::regGetCurrentUserRegistryDword(subKey, "SystemUsesLightTheme", dwordValue))
    {
        // Commented out because this will spam the logs
        // qCDebug(LOG_BASIC) << "Info: Didn't find dark mode registry entry, might be old version of windows";
        return true;
    }
    return dwordValue != 1;
}

} // InterfaceUtils
