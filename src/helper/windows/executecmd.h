#pragma once

#include <Windows.h>
#include <string>

struct ExecuteCmdResult
{
    unsigned long exitCode = 0;
    std::wstring output;
    bool success = false;
};

class ExecuteCmd
{
public:
    static ExecuteCmd &instance()
    {
        static ExecuteCmd i;
        return i;
    }

    void release() {}

    ExecuteCmdResult executeBlockingCmd(const std::wstring &cmd, HANDLE user_token = INVALID_HANDLE_VALUE);
    ExecuteCmdResult executeNonblockingCmd(const std::wstring &cmd, const std::wstring &workingDir);

private:
    ExecuteCmd() = default;

    BOOL isTokenElevated(HANDLE handle);
    void safeCloseHandle(HANDLE handle);

    //  convert std::string to std::wstring with automatic encoding detection
    std::wstring toWString(const std::string &input);
};
