#include "logger.h"
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>

void logOut(const char *str, ...)
{
    va_list args;
    va_start (args, str);
    char buf[4096];
    int cnt = vsnprintf(buf, sizeof(buf), str, args);
    va_end (args);
    
    if (cnt >= 0 && cnt < sizeof(buf))
    {
        printf("%s\n", buf);
        //syslog(LOG_NOTICE, buf);
    }
}

