#include "logger.h"
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>

Logger::Logger()
{
    mkdir("/Library/Logs/com.windscribe.helper.macos", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

Logger::~Logger()
{
}

void Logger::out(const char *str, ...)
{
    time_t tNow;
    ::time(&tNow);
    struct tm* ptNow = ::gmtime(&tNow);

    char buf[4096];
    size_t bytesOut = ::strftime(buf, 128, "[%d%m%y %H:%M:%S:000] [service]\t ", ptNow);

    va_list args;
    va_start (args, str);
    bytesOut += vsnprintf(buf + bytesOut, sizeof(buf) - bytesOut, str, args);
    va_end (args);

    if ((bytesOut > 0) && (bytesOut < sizeof(buf)))
    {
        mutex_.lock();
        FILE* logFile = fopen("/Library/Logs/com.windscribe.helper.macos/helper_log.txt", "a");
        if (logFile != NULL)
        {
            fprintf(logFile, "%s\n", buf);
            fflush(logFile);
            fclose(logFile);
        }
        mutex_.unlock();
    }
}