#ifndef INSTALLHELPER_MAC_H
#define INSTALLHELPER_MAC_H

#include <QString>

// functions for install helper
class InstallHelper_mac
{
public:
    static bool installHelper();
    static bool runScriptWithAdminRights(const QString &scriptPath);
};

#endif // INSTALLHELPER_MAC_H
