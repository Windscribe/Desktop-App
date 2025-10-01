#include "executecmd.h"

#include <optional>

#include <spdlog/spdlog.h>
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

ExecuteCmdResult ExecuteCmd::executeBlockingCmd(const std::wstring &cmd, HANDLE user_token)
{
    ExecuteCmdResult res;

    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa,sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    wsl::Win32Handle pipeRead;
    wsl::Win32Handle pipeWrite;
    if (!CreatePipe(pipeRead.data(), pipeWrite.data(), &sa, 0)) {
        spdlog::error("executeBlockingCmd CreatePipe failed: {}", ::GetLastError());
        return res;
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
        GetExitCodeProcess(pi.hProcess, &res.exitCode);

        pipeWrite.closeHandle();
        res.output = toWString(Utils::readAllFromPipe(pipeRead.getHandle()));

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        res.success = true;
    }
    else {
        spdlog::error("executeBlockingCmd CreateProcess failed: {}", ::GetLastError());
    }

    return res;
}

ExecuteCmdResult ExecuteCmd::executeUnblockingCmd(const std::wstring &cmd, const wchar_t *szEventName, const std::wstring &workingDir)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // As per the Win32 docs; the Unicode version of CreateProcess 'may' modify its lpCommandLine
    // parameter.  Therefore, this parameter cannot be a pointer to read-only memory (such as a
    // const variable or a literal string).  If this parameter is a constant string, CreateProcess
    // may cause an access violation.  Maximum length of the lpCommandLine parameter is 32767.
    std::unique_ptr<wchar_t[]> exec(new wchar_t[32767]);
    wcsncpy_s(exec.get(), 32767, cmd.c_str(), _TRUNCATE);

    ExecuteCmdResult res;

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

        res.success = true;
        res.blockingCmdId = blockingCmdId_;
        blockingCmdId_++;

        blockingCmds_.push_back(blockingCmd);

        RegisterWaitForSingleObject(&blockingCmd->hWait, blockingCmd->hProcess, waitOrTimerCallback,
                                    blockingCmd, INFINITE, WT_EXECUTEONLYONCE);
    }
    else
    {
        spdlog::error("Failed create process: {}", GetLastError());
        delete blockingCmd;
    }

    return res;
}

ExecuteCmdResult ExecuteCmd::getUnblockingCmdStatus(unsigned long cmdId)
{
    std::lock_guard<std::mutex> lock(mutex_);

    ExecuteCmdResult res;
    for (auto it = blockingCmds_.begin(); it != blockingCmds_.end(); ++it)
    {
        if ((*it)->id == cmdId)
        {
            BlockingCmd *blockingCmd = *it;
            if (blockingCmd->bFinished)
            {
                res.success = true;
                res.exitCode = blockingCmd->dwExitCode;
                res.blockingCmdFinished = true;
                res.output = toWString(blockingCmd->strLogOutput);

                blockingCmds_.erase(it);
                delete blockingCmd;
            }
            else
            {
                res.success = true;
                res.blockingCmdFinished = false;
            }
            break;
        }
    }
    return res;
}

ExecuteCmdResult ExecuteCmd::getActiveUnblockingCmdCount()
{
    std::lock_guard<std::mutex> lock(mutex_);
    ExecuteCmdResult res;
    res.success = true;
    res.exitCode = (DWORD)blockingCmds_.size();
    return res;
}

ExecuteCmdResult ExecuteCmd::clearUnblockingCmd(unsigned long id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    clearCmd(id);
    ExecuteCmdResult res;
    res.success = true;
    return res;
}

ExecuteCmdResult ExecuteCmd::suspendUnblockingCmd(unsigned long /*id*/)
{
    std::lock_guard<std::mutex> lock(mutex_);

    ExecuteCmdResult mpr;
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
        }
        else
        {
            spdlog::error("OpenEvent failed, err={}", GetLastError());
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

std::wstring ExecuteCmd::toWString(const std::string &input)
{
    // Automatic encoding detection
    auto tryDecode = [](const std::string& bytes, UINT codePage) -> std::optional<std::wstring> {
        int wideLen = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS,
                                          bytes.data(), bytes.size(), nullptr, 0);
        if (wideLen <= 0) return std::nullopt;

        std::wstring result(wideLen, L'\0');
        if (MultiByteToWideChar(codePage, 0, bytes.data(), bytes.size(),
                                result.data(), wideLen) == 0) {
            return std::nullopt;
        }
        return result;
    };

    // Priorities: UTF-8 -> Console CP -> OEM CP -> ANSI CP
    std::wstring result;
    for (UINT cp : {(UINT)CP_UTF8, GetConsoleOutputCP(), GetOEMCP(), GetACP()}) {
        if (auto decoded = tryDecode(input, cp)) {
            return *decoded;
        }
    }
    return L"Error converting process output to std::wstring";
}
