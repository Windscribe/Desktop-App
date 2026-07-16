#pragma once

#include <Windows.h>
#include <string>

struct ExecuteCmdResult
{
    unsigned long exitCode = 0;
    std::wstring output;
    bool success = false;
    unsigned long processId = 0;
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
    // If outProcessHandle is non-null and the process is created successfully, ownership of the
    // process handle is transferred to the caller (who must CloseHandle it). This lets the caller
    // keep the handle open to prevent the spawned process's PID from being reused while it runs.
    // When outProcessHandle is null the handle is closed immediately (legacy behaviour).
    ExecuteCmdResult executeNonblockingCmd(const std::wstring &cmd, const std::wstring &workingDir, HANDLE *outProcessHandle = nullptr);

private:
    ExecuteCmd() = default;

    //  convert std::string to std::wstring with automatic encoding detection
    std::wstring toWString(const std::string &input);
};
