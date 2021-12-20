#ifndef connections_h
#define connections_h

#include <sys/queue.h>

enum connection_type_t
{
    sflt_connection,         // Connections added by our socket filter
    preexisting_connection,  // Connections added by the IP filter (these are pre-existing connections)
    any_connection           // Either of above
};

// Rebinding behaves differently for Bypass VPN or Only VPN apps in some cases
// when we need to make a trade-off between breaking localhost/LAN and a risk of
// leaks.  This mainly affects UDP, since we need to bind before knowing what
// the app intends to do with the socket.
//
// For Bypass VPN apps, we try to preserve localhost/LAN.  We direct Internet
// traffic to the physical interface, but the app is allowed to communicate over
// the VPN if it explicitly binds to it.
//
// For Only VPN apps, we bind to the tunnel interface more aggressively - this
// breaks LAN/localhost in some cases but minimizes the risk of leaks.
enum RuleType
{
    BypassVPN,
    OnlyVPN
};

extern const uint32_t no_requested_port;

// All addresses/ports are in network byte order.
struct connection_descriptor
{
    char name[PATH_MAX];
    char path[PATH_MAX];
    int pid;
    
    // Once a source address has been observed for a socket (whether we bound it
    // or we observed that it has been bound), this is set, so we'll stop
    // checking this socket (important to avoid a check for every data_in /
    // data_out hook).
    //
    // For IPv4, this always results in source_ip/source_port being set, but for
    // IPv6, we can only store an "any" address.
    boolean_t bound;
    uint32_t source_ip;
    uint32_t source_port;
    uint32_t dest_ip;
    uint32_t dest_port;
    uint32_t bind_ip;
    // If the app attempts to bind, we might defer that bind until we know
    // whether we need to override the address (we can't bind more than once).
    // In that case, this is the port that was requested (which might be 0 if
    // the app attempted to bind to 0.0.0.0:0).  no_requested_port indicates
    // that no bind has occurred.
    uint32_t requested_port;
    uint32_t id;
    enum connection_type_t connection_type;
    enum RuleType rule_type;
    
    // SOCK_STREAM or SOCK_DGRAM (tcp or udp)
    int socket_type;
};

struct conn_entry
{
    TAILQ_ENTRY(conn_entry)        link;
    struct connection_descriptor   desc;
};

struct conn_entry * add_conn(const char *app_path, int pid, uint32_t bind_ip,
                             int socket_type, enum connection_type_t connection_type,
                             enum RuleType rule_type);
void init_conn_list(void);
struct conn_entry * find_conn_by_pid(int pid, enum connection_type_t connection_type);
// Test if a packet matches a known connection (used for the packet filter).
// * If a known connection is bound to 0.0.0.0:<port> (any interface), it
//   matches any source IP.
// * The port must always match.
// * If the specified pid is nonzero, it must match the PID for the known
//   connection.
bool matches_conn(uint32_t source_ip, uint32_t source_port, int pid);
void cleanup_conn_list(void);
void conn_remove(struct conn_entry *entry);

#endif /* connections_h */
