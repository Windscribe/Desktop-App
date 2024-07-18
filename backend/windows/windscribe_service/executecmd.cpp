#include "executecmd.h"

#include "logger.h"
#include "utils.h"
#include "utils/win32handle.h"

unsigned long ExecuteCmd::blockingCmdId_ = 0;
ExecuteCmd *ExecuteCmd::this_ = NULL;

ExecuteCmd::ExecuteCmd()
{
    this_ = this;
}

void ExecuteCmd::release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    clearAllCmds();

    this_ = NULL;
}

MessagePacketResult ExecuteCmd::executeBlockingCmd(const std::wstring &cmd, HANDLE user_token)
{
    MessagePacketResult mpr;

    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa,sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    wsl::Win32Handle pipeRead;
    wsl::Win32Handle pipeWrite;
    if (!CreatePipe(pipeRead.data(), pipeWrite.data(), &sa, 0)) {
        Logger::instance().out(L"executeBlockingCmd CreatePipe failed: %lu", ::GetLastError());
        return mpr;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = NULL;
    si.hStdError = pipeWrite.getHandle();
    si.hStdOutput = pipeWrite.getHandle();

    // As per the Win32 docs; the Unicode version of CreateProcess 'may' modify its lpCommandLine
    // parameter.  Therefore, this parameter cannot be a pointer to read-only memory (such as a
    // const variable or a literal string).  If this parameter is a constant string, CreateProcess
    // may cause an access violation.  Maximum length of the lpCommandLine parameter is 32767.
    std::unique_ptr<wchar_t[]> exec(new wchar_t[32767]);
    wcsncpy_s(exec.get(), 32767, cmd.c_str(), _TRUNCATE);

    ZeroMemory( &pi, sizeof(pi) );
    const auto run_result = user_token != INVALID_HANDLE_VALUE
                                ? CreateProcessAsUser(user_token, NULL, exec.get(), NULL, NULL, TRUE,
                                                      CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi)
                                : CreateProcess(NULL, exec.get(), NULL, NULL, TRUE,
                                                CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);

    if (run_result) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &mpr.exitCode);

        pipeWrite.closeHandle();
        mpr.additionalString = Utils::readAllFromPipe(pipeRead.getHandle());

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        mpr.success = true;
    }
    else {
        Logger::instance().out(L"executeBlockingCmd CreateProcess failed: %lu", ::GetLastError());
    }

    return mpr;
}

MessagePacketResult ExecuteCmd::executeUnblockingCmd(const std::wstring &cmd, const wchar_t *szEventName, const std::wstring &workingDir)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // As per the Win32 docs; the Unicode version of CreateProcess 'may' modify its lpCommandLine
    // parameter.  Therefore, this parameter cannot be a pointer to read-only memory (such as a
    // const variable or a literal string).  If this parameter is a constant string, CreateProcess
    // may cause an access violation.  Maximum length of the lpCommandLine parameter is 32767.
    std::unique_ptr<wchar_t[]> exec(new wchar_t[32767]);
    wcsncpy_s(exec.get(), 32767, cmd.c_str(), _TRUNCATE);

    MessagePacketResult mpr;

    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa,sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    BlockingCmd *blockingCmd = new BlockingCmd();

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = NULL;
    si.hStdError = blockingCmd->pipeForProcess.getPipeHandle();
    si.hStdOutput = blockingCmd->pipeForProcess.getPipeHandle();

    ZeroMemory( &pi, sizeof(pi) );
    if (CreateProcess(NULL, exec.get(), NULL, NULL, TRUE, CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS, NULL, workingDir.c_str(), &si, &pi))
    {
        blockingCmd->pipeForProcess.startReading();
        blockingCmd->szEventName = szEventName;
        blockingCmd->hProcess = pi.hProcess;
        blockingCmd->hThread = pi.hThread;
        blockingCmd->id = blockingCmdId_;

        mpr.success = true;
        mpr.blockingCmdId = blockingCmdId_;
        blockingCmdId_++;

        blockingCmds_.push_back(blockingCmd);

        RegisterWaitForSingleObject(&blockingCmd->hWait, blockingCmd->hProcess, waitOrTimerCallback,
                                    blockingCmd, INFINITE, WT_EXECUTEONLYONCE);
    }
    else
    {
        Logger::instance().out(L"Failed create process: %x", GetLastError());
        delete blockingCmd;
    }

    return mpr;
}

MessagePacketResult ExecuteCmd::getUnblockingCmdStatus(unsigned long cmdId)
{
    std::lock_guard<std::mutex> lock(mutex_);

    MessagePacketResult mpr;
    for (auto it = blockingCmds_.begin(); it != blockingCmds_.end(); ++it)
    {
        if ((*it)->id == cmdId)
        {
            BlockingCmd *blockingCmd = *it;
            if (blockingCmd->bFinished)
            {
                mpr.success = true;
                mpr.exitCode = blockingCmd->dwExitCode;
                mpr.blockingCmdFinished = true;
                mpr.additionalString = blockingCmd->strLogOutput.c_str();

                blockingCmds_.erase(it);
                delete blockingCmd;
            }
            else
            {
                mpr.success = true;
                mpr.blockingCmdFinished = false;
            }
            break;
        }
    }
    return mpr;
}

MessagePacketResult ExecuteCmd::getActiveUnblockingCmdCount()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MessagePacketResult mpr;
    mpr.success = true;
    mpr.exitCode = (DWORD)blockingCmds_.size();
    return mpr;
}

MessagePacketResult ExecuteCmd::clearUnblockingCmd(unsigned long id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    clearCmd(id);
    MessagePacketResult mpr;
    mpr.success = true;
    return mpr;
}

MessagePacketResult ExecuteCmd::suspendUnblockingCmd(unsigned long /*id*/)
{
    std::lock_guard<std::mutex> lock(mutex_);

    MessagePacketResult mpr;
    mpr.success = true;
    return mpr;
}

void ExecuteCmd::waitOrTimerCallback(PVOID lpParameter, BOOLEAN /*timerOrWaitFired*/)
{
    std::lock_guard<std::mutex> lock(this_->mutex_);

    BlockingCmd *blockingCmd = static_cast<BlockingCmd *>(lpParameter);

    DWORD exitCode;
    GetExitCodeProcess(blockingCmd->hProcess, &exitCode);
    blockingCmd->dwExitCode = exitCode;

    blockingCmd->strLogOutput = blockingCmd->pipeForProcess.stopAndGetOutput();

    CloseHandle(blockingCmd->hProcess);
    CloseHandle(blockingCmd->hThread);

    UnregisterWaitEx(blockingCmd->hWait, NULL);

    blockingCmd->bFinished = true;

    // set event if specified
    if (!blockingCmd->szEventName.empty())
    {
        HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, blockingCmd->szEventName.c_str());
        if (hEvent != NULL)
        {
            SetEvent(hEvent);
            CloseHandle(hEvent);
            Logger::instance().out(L"OpenEvent success");
        }
        else
        {
            Logger::instance().out(L"OpenEvent failed, err=%d", GetLastError());
        }
    }
}

void ExecuteCmd::terminateCmd(unsigned long id, unsigned long waitTimeout)
{
    for (auto it = blockingCmds_.begin(); it != blockingCmds_.end(); ++it)
    {
        BlockingCmd *blockingCmd = *it;
        if (blockingCmd->id == id)
        {
            if (!blockingCmd->bFinished)
            {
                TerminateProcess(blockingCmd->hProcess, EXIT_SUCCESS);
                WaitForSingleObject(blockingCmd->hProcess, waitTimeout);
            }
            break;
        }
    }
}

void ExecuteCmd::suspendCmd(unsigned long id)
{
    for (auto it = blockingCmds_.begin(); it != blockingCmds_.end(); ++it)
    {
        BlockingCmd *blockingCmd = *it;
        if (blockingCmd->id == id)
        {
            blockingCmd->pipeForProcess.suspendReading();
            blockingCmd->strLogOutput.clear();
            break;
        }
    }
}

void ExecuteCmd::clearCmd(unsigned long id)
{
    for (auto it = blockingCmds_.begin(); it != blockingCmds_.end(); ++it)
    {
        BlockingCmd *blockingCmd = *it;
        if (blockingCmd->id == id)
        {
            if (!blockingCmd->bFinished)
            {
                CloseHandle(blockingCmd->hProcess);
                CloseHandle(blockingCmd->hThread);

                UnregisterWaitEx(blockingCmd->hWait, NULL);
            }
            blockingCmds_.erase(it);
            delete blockingCmd;
            break;
        }
    }
}

void ExecuteCmd::clearAllCmds()
{
    for (auto it = blockingCmds_.begin(); it != blockingCmds_.end(); ++it)
    {
        BlockingCmd *blockingCmd = *it;
        if (!blockingCmd->bFinished)
        {
            CloseHandle(blockingCmd->hProcess);
            CloseHandle(blockingCmd->hThread);

            UnregisterWaitEx(blockingCmd->hWait, NULL);
        }
        delete blockingCmd;
    }

    blockingCmds_.clear();
}

BOOL ExecuteCmd::isTokenElevated(HANDLE handle)
{
    TOKEN_ELEVATION Elevation;
    DWORD cbSize = sizeof(TOKEN_ELEVATION);
    if (GetTokenInformation(handle, TokenElevation, &Elevation, sizeof(Elevation), &cbSize))
    {
        return Elevation.TokenIsElevated;
    }

    return FALSE;
}

void ExecuteCmd::safeCloseHandle(HANDLE handle)
{
    if (handle)
        CloseHandle(handle);
}
