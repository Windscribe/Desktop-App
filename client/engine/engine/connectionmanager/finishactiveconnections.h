#ifndef FINISHACTIVECONNECTIONS_H
#define FINISHACTIVECONNECTIONS_H

#include "../helper/ihelper.h"

class FinishActiveConnections
{
public:
    static void finishAllActiveConnections(IHelper *helper);

private:
#ifdef Q_OS_WIN
    static void finishAllActiveConnections_win(IHelper *helper);
    static void finishOpenVpnActiveConnections_win(IHelper *helper);
    static void finishIkev2ActiveConnections_win(IHelper *helper);
    static void finishWireGuardActiveConnections_win(IHelper *helper);
#else
    static void finishAllActiveConnections_posix(IHelper *helper);
    static void finishOpenVpnActiveConnections_posix(IHelper *helper);
    static void finishWireGuardActiveConnections_posix(IHelper *helper);
#endif
#if defined Q_OS_LINUX
    static void removeDnsLeaksprotection_linux(IHelper *helper);
#endif
};

#endif // FINISHACTIVECONNECTIONS_H
