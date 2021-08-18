#include <sys/socket.h>
#include <os/log.h>
#include <sys/systm.h>
#include <libkern/OSMalloc.h>
#include "utils.h"

extern OSMallocTag g_osm_tag;

char *strrchr_(const char *s, int c)
{
    const char *found, *p;
    
    c = (unsigned char) c;
    
    /* Since strchr is fast, we use it rather than the obvious loop.  */
    
    if (c == '\0')
        return strchr(s, '\0');
    
    found = NULL;
    while ((p = strchr(s, c)) != NULL)
    {
        found = p;
        s = p + 1;
    }
    
    return (char *) found;
}

char *strncpy_(char *dst, const char*src, size_t n)
{
    char *temp = dst;
    while(n-- && (*dst++ = *src++))
        ;
    
    return temp;
}

char *basename(const char *filename)
{
    char *p = strrchr_(filename, '/');
    return p ? p + 1 : (char *) filename;
}

void store_ip_and_port_addr(const struct sockaddr_in* addr, char *buf, int buf_size)
{
    char addstr[MAX_ADDR_LEN];
    inet_ntop(AF_INET, &addr->sin_addr, (char*)addstr, sizeof(addstr));
    snprintf(buf, buf_size, "%s:%d", addstr, ntohs(addr->sin_port));
}

void store_ip_and_port_addr_6(const struct sockaddr_in6 *addr, char *buf, int buf_size)
{
    char addstr[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &addr->sin6_addr, addstr, sizeof(addstr));
    snprintf(buf, buf_size, "[%s]:%d", addstr, ntohs(addr->sin6_port));
}

bool starts_with(const char *a, const char *b)
{
    if (strncmp(a, b, strlen(b)) == 0)
        return true;
    return false;
}

void *pia_malloc(uint32_t size)
{
    void *address = OSMalloc(size, g_osm_tag);
    
    if(!address)
    {
        log("Cannot allocate memory, OSMalloc failed!");
        return NULL;
    }
    
    bzero(address, size);
    
    return address;
}

void pia_free(void *address, uint32_t size)
{
    OSFree(address, size, g_osm_tag);
}

uint32_t nstol(uint16_t value)
{
    // Widen the value in host byte order
    uint32_t value_host = ntohs(value);
    return htonl(value_host);
}

uint16_t nltos(uint32_t value)
{
    // Truncate in host byte order
    uint16_t value_host = ntohl(value);
    return htons(value_host);
}
