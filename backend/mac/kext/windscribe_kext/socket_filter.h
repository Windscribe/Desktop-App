#ifndef socketFilter_h
#define socketFilter_h

#include <mach/mach_types.h>
#include <sys/kpi_socketfilter.h>

int register_socket_filters(void);
int unregister_socket_filters(void);

#endif // socketFilter_h
