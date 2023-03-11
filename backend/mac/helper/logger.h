#ifndef Logger_h
#define Logger_h

#include <mutex>

#define LOG(str, ...) Logger::instance().out(str, ##__VA_ARGS__)

class Logger
{
public:
    static Logger &instance()
    {
        static Logger i;
        return i;
    }

    void out(const char *format, ...);

private:
    Logger();
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    std::mutex mutex_;
};

#endif // Logger_h
