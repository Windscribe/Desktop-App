#pragma once

#include <xpc/xpc.h>

class Server
{
public:
    Server();
    ~Server();
    void run();

private:
    xpc_connection_t listener_;

    void xpc_event_handler(xpc_connection_t peer);
    void peer_event_handler(xpc_connection_t peer, xpc_object_t event);
};
