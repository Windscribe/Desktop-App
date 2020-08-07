#ifndef LOGGER_H
#define LOGGER_H

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

    bool isFileExist(const std::wstring &fileName);
};

#endif // LOGGER_H
