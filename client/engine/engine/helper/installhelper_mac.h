#ifndef INSTALLHELPER_MAC_H
#define INSTALLHELPER_MAC_H

#include <QString>

// functions for install helper
class InstallHelper_mac
{
public:
    static bool installHelper(bool &isUserCanceled);
    static bool uninstallHelper();
};

#endif // INSTALLHELPER_MAC_H
