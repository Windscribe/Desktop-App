#pragma once

#include <Windows.h>
#include <string>

#include "date_time_helper.h"

class Logger
{
public:
    static Logger &instance()
    {
        static Logger i;
        return i;
    }

    void out(const wchar_t *format, ...);
    void out(const char *format, ...);
    void out(const std::wstring& message);
    void debugOut(const char *format, ...);

private:
    FILE* file_ = nullptr;
    CRITICAL_SECTION cs_;
    DateTimeHelper time_helper_;

    Logger();
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
};
