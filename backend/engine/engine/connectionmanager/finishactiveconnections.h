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
#elif defined Q_OS_MAC
    static void finishAllActiveConnections_mac(IHelper *helper);
    static void finishOpenVpnActiveConnections_mac(IHelper *helper);
    static void finishWireGuardActiveConnections_mac(IHelper *helper);
#endif
};

#endif // FINISHACTIVECONNECTIONS_H
