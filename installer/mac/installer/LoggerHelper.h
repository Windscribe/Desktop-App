#pragma once

class LoggerHelper
{
public:
    LoggerHelper();
    static void writeToLog(const char *str);
    static void logAndStdOut(const char *str);
};
