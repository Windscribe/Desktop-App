#pragma once

#include "../helper/helper.h"

class FinishActiveConnections
{
public:
    static void finishAllActiveConnections(Helper *helper);

private:
#ifdef Q_OS_WIN
    static void finishAllActiveConnections_win(Helper *helper);
    static void finishOpenVpnActiveConnections_win(Helper *helper);
    static void finishIkev2ActiveConnections_win(Helper *helper);
    static void finishWireGuardActiveConnections_win(Helper *helper);
#else
    static void finishAllActiveConnections_posix(Helper *helper);
    static void finishOpenVpnActiveConnections_posix(Helper *helper);
    static void finishWireGuardActiveConnections_posix(Helper *helper);
#endif
#if defined Q_OS_LINUX
    static void removeDnsLeaksprotection_linux(Helper *helper);
#endif
};
