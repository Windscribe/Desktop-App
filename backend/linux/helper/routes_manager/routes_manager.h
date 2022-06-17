#ifndef RoutesManager_h
#define RoutesManager_h

#include "../../../posix_common/helper_commands.h"
#include "routes.h"

// state-based route management
class RoutesManager
{
public:
    RoutesManager();
    void updateState(const CMD_SEND_CONNECT_STATUS &connectStatus);

private:
    CMD_SEND_CONNECT_STATUS connectStatus_;

    Routes dnsServersRoutes_;

    void addDnsRoutes(const CMD_SEND_CONNECT_STATUS &connectStatus);
    void clearAllRoutes();
};

#endif /* RoutesManager_h */
