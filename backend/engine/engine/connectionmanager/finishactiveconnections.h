#ifndef FINISHACTIVECONNECTIONS_H
#define FINISHACTIVECONNECTIONS_H

#include "../helper/ihelper.h"

class FinishActiveConnections
{
public:

    static void finishAllActiveConnections_win(IHelper *helper);


private:
    static void finishOpenVpnActiveConnections_win(IHelper *helper);
    static void finishIkev2ActiveConnections_win(IHelper *helper);
    static void finishWireGuardActiveConnections_win(IHelper *helper);
};

#endif // FINISHACTIVECONNECTIONS_H
