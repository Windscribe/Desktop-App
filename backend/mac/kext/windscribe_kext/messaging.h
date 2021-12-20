#ifndef messaging_h
#define messaging_h

#include <sys/syslimits.h>
#include "connections.h"

enum CommandType
{
    VerifyApp,         // check whether the app should be excluded from VPN
};

typedef struct ProcQuery
{
    enum CommandType command;
    char needs_reply;
    char app_path[PATH_MAX];
    int pid;
    int accept;
    enum RuleType rule_type;
    uint32_t id;
    uint32_t source_ip;
    uint32_t source_port;
    uint32_t dest_ip;
    uint32_t dest_port;
    uint32_t bind_ip;   // the IP to bind to
    
    // SOCK_STREAM or SOCK_DGRAM (tcp or udp)
    int socket_type;
} ProcQuery;

//int send_message_nowait(struct ProcQuery *proc_query);
int send_message_and_wait_for_reply(struct ProcQuery *proc_query, struct ProcQuery *proc_response);

int register_kernel_control(void);
int unregister_kernel_control(void);
boolean_t is_daemon_connected(void);

extern int g_daemon_pid;

#endif /* messaging_h */
