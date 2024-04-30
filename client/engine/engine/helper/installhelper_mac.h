#pragma once

#include <QString>

// functions for install helper
class InstallHelper_mac
{
public:
    static bool installHelper(bool &isUserCanceled);
    static bool uninstallHelper();

private:
    static bool isAppMajorMinorVersionSame();
};
