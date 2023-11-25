#pragma once

#include <mutex>

class Logger
{
public:
    static Logger &instance()
    {
        static Logger i;
        return i;
    }

    void checkLogSize() const;
    void out(const char *format, ...);

private:
    Logger();
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    std::mutex mutex_;
};
