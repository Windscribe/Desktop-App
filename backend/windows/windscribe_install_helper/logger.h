#pragma once

#include <cstdio>

class Logger
{
public:
    explicit Logger(const wchar_t *path);
    virtual ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    void outStr(const wchar_t* format, ...);
    void outCmdLine(int argc, wchar_t* argv[]);

private:
    FILE *file_ = nullptr;
};
