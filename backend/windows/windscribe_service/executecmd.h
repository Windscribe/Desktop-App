#pragma once

#include <Windows.h>

#include <mutex>

#include "ipc/servicecommunication.h"
#include "pipe_for_process.h"

class ExecuteCmd
{
public:
    static ExecuteCmd &instance()
    {
        static ExecuteCmd i;
        return i;
    }

    void release();

    MessagePacketResult executeBlockingCmd(const std::wstring &cmd, HANDLE user_token = INVALID_HANDLE_VALUE);
    MessagePacketResult executeUnblockingCmd(const std::wstring &cmd, const wchar_t *szEventName, const std::wstring &workingDir);
    MessagePacketResult getUnblockingCmdStatus(unsigned long cmdId);
    MessagePacketResult getActiveUnblockingCmdCount();
    MessagePacketResult clearUnblockingCmd(unsigned long id);
    MessagePacketResult suspendUnblockingCmd(unsigned long id);

private:
    ExecuteCmd();

private:
    static unsigned long blockingCmdId_;

    struct BlockingCmd
    {
        unsigned long id;
        std::wstring szEventName;

        PipeForProcess pipeForProcess;
        HANDLE hProcess;
        HANDLE hThread;
        HANDLE hWait;

        bool bFinished;
        DWORD  dwExitCode;
        std::string strLogOutput;

        BlockingCmd() : id(0), hProcess(0), hThread(0), hWait(0), bFinished(false), dwExitCode(0)
        {
        }
    };

    std::vector<BlockingCmd *> blockingCmds_;
    std::mutex mutex_;

    static VOID CALLBACK waitOrTimerCallback(PVOID lpParameter, BOOLEAN timerOrWaitFired);
    static ExecuteCmd *this_;

    void terminateCmd(unsigned long id, unsigned long waitTimeout);
    void suspendCmd(unsigned long id);
    void clearCmd(unsigned long id);
    void clearAllCmds();

    BOOL isTokenElevated(HANDLE handle);
    void safeCloseHandle(HANDLE handle);
};
