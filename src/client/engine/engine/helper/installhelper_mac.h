#pragma once

#include <QString>
#include <spdlog/spdlog.h>

// Functions for install helper
// This class is shared by the mac installer, so logging is done via spdlog
class InstallHelper_mac
{
public:
    static bool installHelper(bool bForceDeleteOld, bool &isUserCanceled, spdlog::logger *logger);
    static bool uninstallHelper(spdlog::logger *logger);

private:
    static bool isAppMajorMinorVersionSame();
};
