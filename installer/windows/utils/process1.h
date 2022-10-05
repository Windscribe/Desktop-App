#ifndef PROCESS1_H
#define PROCESS1_H

#include <Windows.h>

#include <string>
#include <optional>


enum TExecWait { ewNoWait, ewWaitUntilTerminated, ewWaitUntilIdle };

class Process
{
public:
    Process();
    static bool InstExec(const std::wstring Filename, const std::wstring Params,
        std::wstring WorkingDir, const TExecWait Wait, const WORD ShowCmd,
        unsigned long& ResultCode);

    // Run appName with the given commandLine and showWindowFlags.  Wait timeoutMS milliseconds for
    // the process to complete.  Use timeoutMS=INFINITE to wait forever and timeoutMS=0 to not wait.
    // Returns std::nullopt if there was a failure creating the process, waiting for it to complete,
    // or obtaining its exit code.  Returns the process exit code or WAIT_TIMEOUT otherwise.
    static std::optional<DWORD> InstExec(const std::wstring& appName, const std::wstring& commandLine,
                                         DWORD timeoutMS, WORD showWindowFlags);

private:
    static void HandleProcessWait(HANDLE ProcessHandle, const TExecWait Wait, unsigned long& ResultCode);
    static std::wstring extractFileDir(std::wstring path);
};

#endif // PROCESS1_H
