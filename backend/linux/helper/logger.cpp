#include "logger.h"
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>

Logger::Logger()
{
}

Logger::~Logger()
{
}

void Logger::out(const char *str, ...)
{
    time_t tNow;
    ::time(&tNow);
    struct tm* ptNow = ::localtime(&tNow);

    char buf[4096];
    size_t bytesOut = ::strftime(buf, 128, "%d%m%C %H:%M:%S ", ptNow);

    va_list args;
    va_start (args, str);
    bytesOut += vsnprintf(buf + bytesOut, sizeof(buf) - bytesOut, str, args);
    va_end (args);

    if ((bytesOut > 0) && (bytesOut < sizeof(buf)))
    {
        mutex_.lock();
        FILE* logFile = fopen("/usr/local/windscribe/helper_log.txt", "a");
        if (logFile != NULL)
        {
            fprintf(logFile, "%s\n", buf);
            fflush(logFile);
            fclose(logFile);
        }
        mutex_.unlock();
    }
}

void Logger::checkLogSize() const
{
    // TODO: add logic to trim out old entries when the log file gets to >= N megabytes in size.
}
