
#ifndef mutexes_h
#define mutexes_h
#include <sys/systm.h>

extern lck_mtx_t               *g_message_mutex;
extern lck_mtx_t               *g_connection_mutex;

int setup_mutexes(void);
void cleanup_mutexes(void);

#endif // mutexes_h
