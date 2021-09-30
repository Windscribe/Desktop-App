#include "interfaceutils.h"
#include "utils/winutils.h"
#include "utils/logger.h"

namespace InterfaceUtils
{

bool isDarkMode()
{
    int dwordValue = 0;
    const QString subKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    if (!WinUtils::regGetCurrentUserRegistryDword(subKey, "SystemUsesLightTheme", dwordValue))
    {
        qCDebug(LOG_BASIC) << "Error: failed to get dark mode value from registry";
        Q_ASSERT(false);
        return false;
    }
    return dwordValue != 1;
}

} // InterfaceUtils
