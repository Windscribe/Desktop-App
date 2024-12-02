#include "interfaceutils.h"
#include "utils/winutils.h"

namespace InterfaceUtils
{

bool isDarkMode()
{
    int dwordValue = 0;
    const QString subKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    if (!WinUtils::regGetCurrentUserRegistryDword(subKey, "SystemUsesLightTheme", dwordValue))
    {
        return true;
    }
    return dwordValue != 1;
}

} // InterfaceUtils
