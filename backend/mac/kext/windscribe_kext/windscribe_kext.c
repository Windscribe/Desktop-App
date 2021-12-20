#include <mach/mach_types.h>
#include <sys/kern_control.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <netinet/kpi_ipfilter.h>

#include "windscribe_kext.h"
#include "utils.h"
#include "messaging.h"
#include "mutexes.h"
#include "socket_filter.h"
#include "connections.h"


kern_return_t WindscribeKext_start(kmod_info_t * ki, void *d)
{
    init_conn_list();
    
    if (setup_mutexes())
    {
        goto bail;
    }
    
    if (register_kernel_control())
    {
        goto bail;
    }
    
    if (register_socket_filters())
    {
        goto bail;
    }
    
    log("Windscribe kext started.\n");
    return KERN_SUCCESS;

bail:
    cleanup_mutexes();
    unregister_kernel_control();
    unregister_socket_filters();
    log("Windscribe kext failed to start.\n");

    return KERN_FAILURE;
}

kern_return_t WindscribeKext_stop(kmod_info_t *ki, void *d)
{
    if (unregister_kernel_control() )
    {
        log("Windscribe kext failed to stop.\n");
        return EBUSY;
    }
    
    if (unregister_socket_filters())
    {
        log("Windscribe kext failed to stop.\n");
        return EBUSY;
    }
    
    cleanup_conn_list();
    cleanup_mutexes();
    
    log("Windscribe kext stopped.\n");
    return KERN_SUCCESS;
}
