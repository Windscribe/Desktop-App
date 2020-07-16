#include "mutexes.h"
#include "windscribe_kext.h"
#include <libkern/OSMalloc.h>
//#include <mach/mach_types.h>
//#include <sys/kern_control.h>
//#include <sys/errno.h>


OSMallocTag             g_osm_tag = NULL;
lck_grp_t               *g_mutex_group = NULL;
lck_mtx_t               *g_message_mutex = NULL;
lck_mtx_t               *g_connection_mutex = NULL;

int setup_mutexes(void)
{
    g_osm_tag = OSMalloc_Tagalloc(BUNDLE_ID, OSMT_DEFAULT);
    if (!g_osm_tag)
        return -1;
    
    // allocate mutex group and a mutexes to protect global data.
    g_mutex_group = lck_grp_alloc_init(BUNDLE_ID, LCK_GRP_ATTR_NULL);
    if (!g_mutex_group)
        return -1;
    
    g_connection_mutex = lck_mtx_alloc_init(g_mutex_group, LCK_ATTR_NULL);
    if(!g_connection_mutex)
        return -1;
    
    g_message_mutex = lck_mtx_alloc_init(g_mutex_group, LCK_ATTR_NULL);
    if (!g_message_mutex)
        return -1;
    
    /*g_firewall_mutex = lck_mtx_alloc_init(g_mutex_group, LCK_ATTR_NULL);
    if(!g_firewall_mutex)
        return -1;*/
    
    return 0;
}

void cleanup_mutexes(void)
{
    if(g_connection_mutex) lck_mtx_free(g_connection_mutex, g_mutex_group);
    if (g_message_mutex) lck_mtx_free(g_message_mutex, g_mutex_group);
    //if(g_firewall_mutex) lck_mtx_free(g_firewall_mutex, g_mutex_group);
    if (g_mutex_group) lck_grp_free(g_mutex_group);
    if (g_osm_tag) OSMalloc_Tagfree(g_osm_tag);
    
    g_connection_mutex = NULL;
    g_message_mutex = NULL;
    //g_firewall_mutex = NULL;
    g_mutex_group = NULL;
    g_osm_tag = NULL;
}
