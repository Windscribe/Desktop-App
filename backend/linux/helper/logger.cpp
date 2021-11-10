#include "logger.h"
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>

// TODO: must make this thread safe
void logOut(const char *str, ...)
{
    va_list args;
    va_start (args, str);
    char buf[4096];
    int cnt = vsnprintf(buf, sizeof(buf), str, args);
    va_end (args);
    
    if (cnt >= 0 && static_cast<long unsigned int>(cnt) < sizeof(buf))
    {
        FILE* logFile = fopen("/usr/local/windscribe/helper_log.txt", "w+");
        if (logFile != NULL)
        {
            fprintf(logFile, "%s\n", buf);
            fflush(logFile);
            fclose(logFile);
        }

        //syslog(LOG_NOTICE, buf);
    }
}

