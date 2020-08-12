#include "all_headers.h"
#include "pipe_for_process.h"
#include "ipc/servicecommunication.h"
#include "executecmd.h"
#include "utils.h"
#include "logger.h"

unsigned long ExecuteCmd::blockingCmdId_ = 0;
ExecuteCmd *ExecuteCmd::this_ = NULL;

ExecuteCmd::ExecuteCmd()
{
    this_ = this;
}

ExecuteCmd::~ExecuteCmd()
{
    std::lock_guard<std::mutex> lock(mutex_);
    clearAllCmds();

    this_ = NULL;
}

MessagePacketResult ExecuteCmd::executeBlockingCmd(wchar_t *cmd)
{
    MessagePacketResult mpr;

    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa,sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE rPipe, wPipe;
    if (!CreatePipe(&rPipe, &wPipe, &sa, 0))
    {
        return mpr;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = NULL;
    si.hStdError = wPipe;
    si.hStdOutput = wPipe;

    ZeroMemory( &pi, sizeof(pi) );
	if (CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
    {
        DWORD exitCode;
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(wPipe);
        std::string str = Utils::readAllFromPipe(rPipe);
        CloseHandle(rPipe);

        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );

        mpr.success = true;
        mpr.exitCode = exitCode;
        mpr.sizeOfAdditionalData = (DWORD)str.length();
        if (str.length() > 0)
        {
            mpr.szAdditionalData = new char[str.length()];
			memcpy(mpr.szAdditionalData, str.c_str(), str.length());
        }
    }
    else
    {
		Logger::instance().out(L"Failed create process: %x", GetLastError());
        CloseHandle(rPipe);
        CloseHandle(wPipe);
    }
    return mpr;
}

MessagePacketResult ExecuteCmd::executeUnblockingCmd(const wchar_t *cmd, const wchar_t *szEventName, const wchar_t *szWorkingDir)
{
    std::lock_guard<std::mutex> lock(mutex_);

	wchar_t szCmd[MAX_PATH];

	wcscpy(szCmd, cmd);

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
	if (CreateProcess(NULL, szCmd, NULL, NULL, TRUE, CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS, NULL, szWorkingDir, &si, &pi))
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
                mpr.sizeOfAdditionalData = (DWORD)blockingCmd->strLogOutput.length();
                mpr.szAdditionalData = new char[blockingCmd->strLogOutput.length()];
                memcpy(mpr.szAdditionalData, blockingCmd->strLogOutput.c_str(), blockingCmd->strLogOutput.length());

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
