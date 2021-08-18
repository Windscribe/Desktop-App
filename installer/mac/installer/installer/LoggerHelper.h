#ifndef LoggerHelper_h
#define LoggerHelper_h

class LoggerHelper
{
public:
    LoggerHelper();
    static void writeToLog(const char *str);
    static void logAndStdOut(const char *str);
};

#endif /* LoggerHelper_h */
