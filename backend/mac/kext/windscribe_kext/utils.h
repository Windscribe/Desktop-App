#ifndef utils_h
#define utils_h
#include <os/log.h>
#include <netinet/in.h>

char * strrchr_(const char *s, int c);

// We're implementing our own strncpy here for 10.12 compatibility.
// The one provided by Apple relies on a symbol (__strncpy_chk) that doesn't exist on 10.12
char * strncpy_(char *dst, const char*src, size_t n);
char * basename(const char *filename);
void store_ip_and_port_addr(const struct sockaddr_in* addr, char *buf, int buf_size);
// Length of buffer for store_ip_and_port_addr_6 to avoid truncation
// [XX]:##### -> IPv6_len + 8
#define PIA_IN6_ADDRPORT_LEN (INET6_ADDRSTRLEN + 8)
void store_ip_and_port_addr_6(const struct sockaddr_in6 *addr, char *buf, int buf_size);
bool starts_with(const char *a, const char *b);
void pia_free(void *address, uint32_t size);
void *pia_malloc(uint32_t size);

// Widen or truncate network-byte-order values (16 to 32 bits or vice versa)
uint32_t nstol(uint16_t value);
uint16_t nltos(uint32_t value);


#define __FILENAME__ (strrchr_(__FILE__, '/') ? strrchr_(__FILE__, '/') + 1 : __FILE__)

#define log(str, ...) os_log(OS_LOG_DEFAULT, "[%s:%d] " str, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define log_debug(str, ...) os_log_debug(OS_LOG_DEFAULT, "[%s:%d] " str, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define NUM_ELEMENTS(array) (sizeof(array) / sizeof((array)[0]))

#define MAX_ADDR_LEN 256

#endif /* utils_h */
