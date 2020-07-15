#include "socketFilter.h"
#include "utils.h"
#include "WindscribeKext.h"
#include "messaging.h"
#include "connections.h"

#include <stddef.h>

#include <sys/socket.h>
#include <sys/kpi_socket.h>
#include <sys/kpi_mbuf.h>
#include <sys/kpi_socket.h>
#include <sys/kpi_socketfilter.h>

#include <netinet/kpi_ipfilter.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <sys/proc.h>

#define WS_FLT_TCP_HANDLE       'wsc0'
#define WS_FLT_UDP_HANDLE       'wsc1'

static boolean_t        g_filter_tcp_registered = FALSE;
static boolean_t        g_filter_udp_registered = FALSE;


boolean_t is_sockaddr_in(const struct sockaddr *addr)
{
    return addr && addr->sa_family == AF_INET &&
        addr->sa_len >= sizeof(struct sockaddr_in);
}

struct sockaddr_in *as_sockaddr_in(struct sockaddr *addr)
{
    return is_sockaddr_in(addr) ? (struct sockaddr_in*)addr : NULL;
}

const struct sockaddr_in *as_sockaddr_in_c(const struct sockaddr *addr)
{
    return is_sockaddr_in(addr) ? (const struct sockaddr_in*)addr : NULL;
}

static errno_t check_bind4(struct conn_entry *entry,
                           const struct sockaddr_in *to_in)
{
    // Printable address for logging
    char to_in_addr[MAX_ADDR_LEN] = {0};
    store_ip_and_port_addr(to_in, to_in_addr, sizeof(to_in_addr));

    // Bypass VPN apps can bind to any interface, but Only VPN apps cannot
    // (reject it to prevent leaks).
    if (entry->desc.rule_type == BypassVPN)
    {
        log("id %d app binding to %s - allowed (bypass)", entry->desc.id, to_in_addr);
        // Don't update bound/source_ip/source_port yet - we might not know
        // a specific port yet.  Let the listen or connect_out hook observe
        // the result.
        return 0;
    }

    // Only VPN - allow binding to loopback or the address we would have
    // bound to (as long as it's known).  Reject otherwise.
    if (IN_LOOPBACK(ntohl(to_in->sin_addr.s_addr)))
    {
        log("id %d app binding to %s - allowed (only VPN, loopback)", entry->desc.id, to_in_addr);
        return 0;
    }

    if (entry->desc.bind_ip && to_in->sin_addr.s_addr == entry->desc.bind_ip)
    {
        log("id %d app binding to %s - allowed (only VPN, tunnel IP)", entry->desc.id, to_in_addr);
        return 0;
    }

    log("id %d app binding to %s - rejected (only VPN, address not allowed)", entry->desc.id, to_in_addr);
    return ENETUNREACH;
}

static void ws_unregistered(sflt_handle handle)
{
    switch (handle)
    {
        case WS_FLT_TCP_HANDLE:
            log("Unregistered TCP filter (wsc0)");
            g_filter_tcp_registered = FALSE;
            break;
        case WS_FLT_UDP_HANDLE:
            log("Unregistered UDP filter (pia1)");
            g_filter_udp_registered = FALSE;
            break;
        default:
            log("Unregistered UNKNOWN filter with handle %d\n", handle);
            break;
    }
}


static int bind_socket_to_address(uint32_t id, socket_t so, uint16_t port, unsigned int address)
{
    struct sockaddr_in newAddr = {0};
    newAddr.sin_len = sizeof(newAddr); // yes this field is needed
    newAddr.sin_family = AF_INET;
    newAddr.sin_port = port;
    newAddr.sin_addr.s_addr = address;

    char addr_str[MAX_ADDR_LEN] = {0};
    store_ip_and_port_addr(&newAddr, addr_str, sizeof(newAddr));
    
    int err = 0;
    if((err = sock_bind(so, (struct sockaddr*)&newAddr)))
    {
        log("id %d error binding socket to %s - code %d", id, addr_str, err);
    }
    else
    {
        log("id %d successfully bound to %s", id, addr_str);
    }
    
    return err;
}

// Bind a socket that we've attached to to the specified address.
// If entry->desc.requested_port is set, that port is used, otherwise port 0 is
// used.  The entry's source addresses and bound flag are set.
static errno_t bind_attached_socket(socket_t so, struct conn_entry *entry, uint32_t bind_ip)
{
    assert(entry);  // Checked by caller
    // New entry; not rebound yet (ensured by caller)
    assert(!entry->desc.bound);
    assert(entry->desc.source_ip == 0);
    assert(entry->desc.source_port == 0);

    // scratch space for storing addresses for logging
    char addr[MAX_ADDR_LEN] = {0};
    struct sockaddr_in source = {0};

    // Set the bound flag before calling sock_getsockname() or sock_bind(), so
    // hooks won't try to process these calls (possibly recursively).
    entry->desc.bound = true;

    sock_getsockname(so, (struct sockaddr*)&source, sizeof(struct sockaddr));
    store_ip_and_port_addr(&source, addr, sizeof(addr));
    log("id %d Source before rebinding is: %s", entry->desc.id, addr);

    // If the app had asked for a port, apply it now as we bind
    uint16_t port = 0;
    if(entry->desc.requested_port != no_requested_port)
    {
        port = nltos(entry->desc.requested_port);
        entry->desc.requested_port = no_requested_port;
    }

    int bindErr = bind_socket_to_address(entry->desc.id, so, port, bind_ip);
    if(bindErr)
    {
        entry->desc.bound = false;  // Reset; bind failed
        // Error rebinding socket, it's logged in bind_socket_to_address() so
        // let's early-exit
        return bindErr;
    }
    sock_getsockname(so, (struct sockaddr*)&source, sizeof(struct sockaddr));

    entry->desc.source_ip = source.sin_addr.s_addr;
    entry->desc.source_port = source.sin_port;
    entry->desc.dest_ip = 0;
    entry->desc.dest_port = 0;

    store_ip_and_port_addr(&source, addr, sizeof(addr));
    log("id %d Source after rebinding is: %s", entry->desc.id, addr);

    return 0;
}

static errno_t bind_to_interface(socket_t so, struct conn_entry *entry)
{
    if(!entry->desc.bind_ip)
    {
        log("id %d error binding socket: No source address (received 0.0.0.0)", entry->desc.id);
        /* No address, return error code */
        return ENETUNREACH;
    }

    return bind_attached_socket(so, entry, entry->desc.bind_ip);
}

// Apply the requested port if it is set, without binding to a specific address.
// (Used when we've determined that we do not need to override the address.)
static errno_t apply_requested_port(socket_t so, struct conn_entry *entry,
                                    const char *callback_name)
{
    if(entry->desc.requested_port != no_requested_port)
    {
        log("id %d bind to requested port %u - not overriding address (%s)",
            entry->desc.id, ntohl(entry->desc.requested_port), callback_name);
        // Bind to 'any' IP address
        return bind_attached_socket(so, entry, 0);
    }

    return 0;
}

// If an excluded app listens on a socket or receives incoming data, remember
// the bound address so we can permit it in the packet filter.
static errno_t check_inbound_socket(void *cookie, socket_t so,
                                    const char *callback_name)
{
    struct conn_entry* entry = (struct conn_entry *)cookie;

    if(entry && !entry->desc.bound)
    {
        // If a bind was deferred, apply it now.  Don't override the address for
        // inbound sockets.
        int bindErr = apply_requested_port(so, (struct conn_entry *)cookie, callback_name);
        if(bindErr)
            return bindErr;

        char addr[MAX_ADDR_LEN] = {0};
        struct sockaddr_in source = {0};
        sock_getsockname(so, (struct sockaddr*)&source, sizeof(struct sockaddr_in));
        store_ip_and_port_addr(&source, addr, sizeof(addr));

        log("id %d Inbound socket (%s): %s", entry->desc.id, callback_name, addr);
        entry->desc.bound = true;
        entry->desc.source_ip = source.sin_addr.s_addr;
        entry->desc.source_port = source.sin_port;
    }

    return 0;
}

// Check a socket to see if we should bind it to bind_ip.
//
// The socket is bound if all of the following are true:
// - The socket is connecting to an IPv4 address
// - The socket belongs to an app with a matching split tunnel rule (cookie is valid)
// - The socket hasn't been previously bound or observed as bound (avoids
//   calling sock_getsockname() for every UDP data packet)
// - The socket isn't connecting to a loopback address.
static errno_t check_socket_rebind(void *cookie, socket_t so,
                                   const struct sockaddr *to,
                                   const char *callback_name)
{
    struct conn_entry* entry = (struct conn_entry *)cookie;
    if(!entry || entry->desc.bound)
        return 0;

    // If the socket isn't connecting to an IPv4 address, we can't do anything
    // with it.
    const struct sockaddr_in *to_in = as_sockaddr_in_c(to);
    if(!to_in)
    {
        log("id %d invalid to address", entry->desc.id);
        return 0;
    }

    // If this socket belongs to a matching app, pia_attach() created a
    // connection entry and set it as the cookie.
    //
    // If it's a matching app, and hasn't already been bound, check if we
    // should rebind it based on the destination address.
    {
        // Check if the socket was already bound
        char addr[MAX_ADDR_LEN] = {0};
        struct sockaddr_in source = {0};
        // Ensure our getsockname() hook doesn't do anything
        entry->desc.bound = true;
        sock_getsockname(so, (struct sockaddr*)&source, sizeof(struct sockaddr));
        // If the socket already has a bound source, update our connection
        // entry, don't try to bind again.  (The kernel never allows more than
        // one bind().)
        if(source.sin_addr.s_addr || source.sin_port)
        {
            store_ip_and_port_addr(&source, addr, sizeof(addr));
            log("id %d Already bound (%s): %s", entry->desc.id, callback_name, addr);
            entry->desc.source_ip = source.sin_addr.s_addr;
            entry->desc.source_port = source.sin_port;
            // If, somehow, we still have a requested port, then we failed to apply it
            if(entry->desc.requested_port != no_requested_port)
            {
                log("id %d Missed applying requested port %u!", entry->desc.id,
                    ntohl(entry->desc.requested_port));
            }
            return 0;
        }

        // Not bound yet; reset
        entry->desc.bound = false;

        // If the destination address is loopback, do not bind to the
        // physical interface.
        if(IN_LOOPBACK(ntohl(to_in->sin_addr.s_addr)))
        {
            store_ip_and_port_addr(to_in, addr, sizeof(addr));
            log("id %d Connecting to localhost (%s): %s", entry->desc.id, callback_name, addr);
            // Don't override address, just apply any deferred bind to a port
            return apply_requested_port(so, entry, callback_name);
        }

        log("id %d Binding (%s)", entry->desc.id, callback_name);
        // This is a matching app, bind to the specified address.
        return bind_to_interface(so, entry);
    }
}

static errno_t ws_attach(void **cookie, socket_t so)
{
    char pathbuf[PATH_MAX];
    char                 name[PATH_MAX];
    int pid              = 0;
    int socket_type      = 0;
    int err              = 0;
    
    *cookie = NULL;
    
    
    // Do not attach if daemon is not connected
    if (!is_daemon_connected())
        return ENOPOLICY;
    
    // Do not attach to daemon sockets OR the kernel (pid == 0)
    if(proc_selfpid() == g_daemon_pid || proc_selfpid() == 0)
        return ENOPOLICY;
    
    // Is our socket TCP or UDP?
    if((err = sock_gettype(so, NULL, &socket_type, NULL)))
    {
        log("Error: Could not get socket type: code %d\n", err);
        
        // Give up
        return ENOPOLICY;
    }
    
    proc_selfname(name, PATH_MAX);
    pid = proc_selfpid();
    
    proc_name(pid, pathbuf, sizeof(pathbuf));

    {
        ProcQuery proc_query = { .command = VerifyApp, .pid = proc_selfpid(), .socket_type = socket_type };
        ProcQuery proc_response = {0};
        
        if (send_message_and_wait_for_reply(&proc_query, &proc_response))
            return ENOPOLICY;
        
        if (!proc_response.accept)
        {
            // Verification was denied (the process was not in the exclusions list)
            // so we do not bind to this socket
            return ENOPOLICY;
        }
        
        // Add the connection to our connections list (this entry also serves as the cookie for our socket filter)
        struct conn_entry *entry = add_conn(proc_response.app_path, proc_response.pid,
                                            proc_response.bind_ip, socket_type,
                                            sflt_connection,
                                            proc_response.rule_type);
        
        if(!entry)
            return ENOPOLICY;

        log("id %d Attaching to %s socket. name: %s, pid: %d\n", entry->desc.id, socket_type == SOCK_DGRAM ? "UDP" : "TCP", name, pid);

        *cookie = (void*)entry;
    }
    
    return 0;
}

static void ws_detach(void *cookie, socket_t so)
{
    if (cookie)
    {
        struct conn_entry* entry = (struct conn_entry *)cookie;
        log("id %d Detach %s app_entry is: name: %s, pid: %d\n", entry->desc.id, entry->desc.socket_type == SOCK_DGRAM ? "UDP" : "TCP", entry->desc.name, entry->desc.pid);
        
        // This removes the entry from the list of connections
        // and frees the memory associated with the entry.
        conn_remove(entry);
    }
    else
    {
        log("No cookie was found for this socket!\n");
    }
    
    cookie = NULL;
    
    return;
}

static int ws_getsockname_tcp4(void *cookie, socket_t so, struct sockaddr **sa)
{
    struct conn_entry* entry = (struct conn_entry*)cookie;
    if(!entry || entry->desc.bound)
        return 0;
    struct sockaddr_in *sa_in = as_sockaddr_in(sa ? *sa : NULL);
    if(!sa_in)
    {
        log("id %d invalid getsockname address buffer", entry->desc.id);
        return 0;
    }

    // An app may call getsockname() after bind() - in which case it expects to
    // observe the result of the bind().
    if(entry->desc.requested_port != no_requested_port)
    {
        // If a specific port was requested, we can fake the response.
        if(entry->desc.requested_port != 0)
        {
            log("id %d getsockname - fake response with requested port %u",
                entry->desc.id, ntohl(entry->desc.requested_port));
            // The app requested 0.0.0.0.  It's possible we might override this
            // to a specific address, but the app shouldn't care since it asked
            // for 0.0.0.0.
            sa_in->sin_addr.s_addr = 0;
            sa_in->sin_port = nltos(entry->desc.requested_port);
            return EJUSTRETURN;
        }

        // Otherwise, the app tried to bind to 0.0.0.0:0, and it now wants to
        // know what port it got.  We can't fake this, make our best guess and
        // bind now.
        //
        // It's probably going to listen on this socket, so for Bypass VPN apps,
        // don't bind to an interface.  For Only VPN apps, bind to the interface
        // anyway to avoid leaks, since we're not sure.
        if(entry->desc.rule_type == BypassVPN)
        {
            log("id %d bind to any interface now due to getsockname", entry->desc.id);
            apply_requested_port(so, entry, "getsockname_tcp4");
        }
        else
        {
            log("id %d bind to interface now due to getsockname", entry->desc.id);
            bind_to_interface(so, entry);
        }
    }

    return 0;
}

static errno_t ws_data_in(void *cookie, socket_t so, const struct sockaddr *from,
                           mbuf_t *data, mbuf_t *control, sflt_data_flag_t flags)
{
    return check_inbound_socket(cookie, so, "data_in");
}

static errno_t ws_data_out(void *cookie, socket_t so, const struct sockaddr *to,
                            mbuf_t *data, mbuf_t *control, sflt_data_flag_t flags)
{
    // This isn't completely correct - a UDP socket could be used to send to
    // multiple hosts, even combinations of localhost/LAN/Internet hosts.
    // We should eventually rewrite the individual packets based on the remote
    // host, but for now we're still rebinding the socket based on the first
    // outbound data packet.
    return check_socket_rebind(cookie, so, to, "data_out");
}

static errno_t ws_connect_in(void *cookie, socket_t so, const struct sockaddr *from)
{
    // Often, inbound TCP connections have already been handled by listen(), but
    // this filter might be the first one to observe a socket if the app was
    // already listening when PIA was started.
    return check_inbound_socket(cookie, so, "connect_in");
}

static errno_t ws_connect_out(void *cookie, socket_t so, const struct sockaddr *to)
{
    return check_socket_rebind(cookie, so, to, "connect_out");
}

static errno_t ws_bind_tcp4(void *cookie, socket_t so, const struct sockaddr *to)
{
    struct conn_entry* entry = (struct conn_entry*)cookie;
    // If the entry is already bound, this may mean that we're doing the bind
    // right now - we get this hook even when the kext binds a socket.
    if(!entry || entry->desc.bound)
        return 0;

    // The bind address should be IPv4; this is only used as an IPv4 hook.
    const struct sockaddr_in *to_in = as_sockaddr_in_c(to);
    if (!to_in)
    {
        log("id %d invalid bind address", entry->desc.id);
        return 0;
    }

    // If a specific address was requested, don't defer anything.  We never
    // _change_ a specific requested address, but we might block it entirely if
    // the app shouldn't be communicating on this interface.
    if(to_in->sin_addr.s_addr)
        return check_bind4(entry, to_in);

    // Printable address for logging
    char addr[MAX_ADDR_LEN] = {0};
    store_ip_and_port_addr(to_in, addr, sizeof(addr));

    // The app didn't request a specific address.  Defer the bind until we know
    // whether we should override the address.  (The port could be 0 if the app
    // didn't request a specific port, this is still deferred.)
    log("id %d app binding to %s - deferred", entry->desc.id, addr);
    entry->desc.requested_port = nstol(to_in->sin_port);
    // Return EJUSTRETURN - the kernel will ignore the bind but proceed normally
    return EJUSTRETURN;
}

static errno_t ws_listen_tcp4(void *cookie, socket_t so)
{
    return check_inbound_socket(cookie, so, "listen");
}

static errno_t ws_bind_udp4(void *cookie, socket_t so, const struct sockaddr *to)
{
    struct conn_entry* entry = (struct conn_entry*)cookie;
    if(!entry || entry->desc.bound)
        return 0;

    // The bind address should be IPv4; this is only used as an IPv4 hook.
    const struct sockaddr_in *to_in = as_sockaddr_in_c(to);
    if(!to_in)
    {
        log("id %d invalid bind address", entry->desc.id);
        return 0;
    }

    // If a specific address was requested, just check it.
    if(to_in->sin_addr.s_addr)
        return check_bind4(entry, to_in);

    char addr[MAX_ADDR_LEN] = {0};
    store_ip_and_port_addr(to_in, addr, sizeof(addr));

    // The app didn't request a specific address.  We can't defer a UDP bind;
    // the app might just wait for incoming data after this.  We have to make
    // our best guess now.  For Bypass VPN apps, just allow it; don't change
    // anything - this socket is probably intended for incoming traffic, and
    // this preserves localhost/LAN connectivity.
    if(entry->desc.rule_type == BypassVPN)
    {
        log("id %d app binding to %s - allowed (bypass)", entry->desc.id, addr);
        return 0;
    }

    // For Only VPN apps, override with the bind IP - don't risk allowing these
    // to leak.
    log("id %d app binding to %s - override (only VPN)", entry->desc.id, addr);
    entry->desc.requested_port = nstol(to_in->sin_port);
    int bindErr = bind_to_interface(so, entry);
    if(bindErr)
        return bindErr;
    // We already did the bind, tell the kernel just to return.
    return EJUSTRETURN;
}

// TCP socket hooks
//
// For TCP (IPv4), we bind to the specified address in connect(), depending on
// the remote address.
//
// If the app attempts to bind to a specific port (without a specific address),
// we defer the bind until connect() since we can't bind more than once, and
// we don't know what the remote address would be yet.
static struct sflt_filter socket_tcp_filter = {
    WS_FLT_TCP_HANDLE,
    SFLT_GLOBAL,
    BUNDLE_ID,
    ws_unregistered,
    ws_attach,
    ws_detach,
    NULL,
    NULL,
    ws_getsockname_tcp4,
    ws_data_in,
    ws_data_out,
    ws_connect_in,
    ws_connect_out,
    ws_bind_tcp4,
    NULL,
    NULL,
    ws_listen_tcp4,
    NULL
};

// UDP socket hooks
//
// UDP binding depends on the rule type, since it involves tradeoffs between
// LAN/localhost/inbound functionality, and risk of leaks.
//
// For Only VPN apps, we bind if the app binds, or on the first outbound data
// packet otherwise.  (Inbound data won't be the first event, a bind would have
// to occur.)  This breaks localhost traffic, but binding on data out risks
// leaks if an inbound packet occurs first, or if the app binds to a specific
// port.  (We also can't defer the bind like TCP, since the app might bind and
// then do nothing, expecting incoming packets.)
//
// For Bypass VPN apps, we bind only on outbound data.  This preserves localhost
// and inbound functionality, but could possibly introduce a leak as above.  A
// leak of a Bypass VPN app onto the VPN is less of an issue than vice versa,
// and inbound/localhost are important to some apps that are often used with
// Bypass VPN.
//
// (UDP might be better implemented with per-packet rewriting eventually, since
// in principle the same socket could be used for any combination of
// LAN/localhost/Internet and inbound/outbound, unlike TCP.)
static struct sflt_filter socket_udp_filter = {
    WS_FLT_UDP_HANDLE,
    SFLT_GLOBAL,
    BUNDLE_ID,
    ws_unregistered,
    ws_attach,
    ws_detach,
    NULL,
    NULL,
    NULL,
    ws_data_in,
    ws_data_out,
    NULL,
    ws_connect_out,
    ws_bind_udp4,
    NULL,
    NULL,
    NULL,
    NULL
};

int register_socket_filter(struct sflt_filter *filter, int domain, int type, int protocol, boolean_t *registered, const char *name)
{
    int ret;
    if ((ret = sflt_register(filter, domain, type, protocol)))
    {
        log("Could not register %s socket filter, error code %d\n", name, ret);
    }
    else
    {
        *registered = TRUE;
    }
    return ret;
}

int register_socket_filters(void)
{
    int ret;
    ret = register_socket_filter(&socket_tcp_filter, PF_INET, SOCK_STREAM, IPPROTO_TCP, &g_filter_tcp_registered, "tcp");
    if (ret)
    {
        return ret;
    }
    ret = register_socket_filter(&socket_udp_filter, PF_INET, SOCK_DGRAM, IPPROTO_UDP, &g_filter_udp_registered, "udp");
    return ret;
}

void unregister_socket_filter(boolean_t *registered, boolean_t *started,
                              sflt_handle handle, const char *name)
{
    if(*registered == TRUE && !*started)
    {
        errno_t error_sflt = sflt_unregister(handle);
        if(error_sflt)
            log("Exit code for sflt_unregister %s is %d\n", name, error_sflt);
        *started = TRUE;
    }
}

void check_socket_unregistered(boolean_t *registered, int *result, const char *name)
{
    if(*result == 0 && *registered)
    {
        log("The %s socket filter is still registered", name);
        *result = -1;
    }
}

int unregister_socket_filters(void)
{
    static boolean_t filter_tcp_unregister_started = FALSE;
    static boolean_t filter_udp_unregister_started = FALSE;
    
    unregister_socket_filter(&g_filter_tcp_registered, &filter_tcp_unregister_started, WS_FLT_TCP_HANDLE, "tcp");
    unregister_socket_filter(&g_filter_udp_registered, &filter_udp_unregister_started, WS_FLT_UDP_HANDLE, "udp");
    
    int result = 0;
    check_socket_unregistered(&g_filter_tcp_registered, &result, "TCP");
    check_socket_unregistered(&g_filter_udp_registered, &result, "UDP");
    
    return result;
}
