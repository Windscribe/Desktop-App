#ifndef LOGGER_H
#define LOGGER_H

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


private:
    Logger();
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    FILE *file_;
    CRITICAL_SECTION cs_;
    DateTimeHelper time_helper_;

    bool isFileExist(const std::wstring &fileName);
};

#endif // LOGGER_H
