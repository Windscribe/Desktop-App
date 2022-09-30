#include "process1.h"

#include <sstream>

#include "logger.h"
#include "../../../../../client/common/utils/wsscopeguard.h"

using namespace std;


Process::Process()
{
}

void Process::HandleProcessWait(HANDLE ProcessHandle, const TExecWait Wait, unsigned long& ResultCode)
{
    /*if(Wait == ewWaitUntilIdle)
       {
         while(1)
         {
          if(WaitForInputIdle(ProcessHandle, 50) != WAIT_TIMEOUT) break;
         }
       }

    if(Wait == ewWaitUntilTerminated)
       {
        while(1)
         {
          DWORD ret;
          ret = MsgWaitForMultipleObjects(1, &ProcessHandle, false, INFINITE, QS_ALLINPUT);
         if(ret != (WAIT_OBJECT_0+1)) break;
         }
      }*/

    WaitForSingleObject(ProcessHandle, INFINITE);
    // { Get the exit code. Will be set to STILL_ACTIVE if not yet available }
    if (GetExitCodeProcess(ProcessHandle, &ResultCode) == false)
    {
        ResultCode = 0xFFFFFFFF;  //{ just in case }
    }

    CloseHandle(ProcessHandle);
}

bool Process::InstExec(const wstring Filename, const wstring Params,
    wstring WorkingDir, const TExecWait Wait, const WORD ShowCmd,
    unsigned long& ResultCode)
{
    wstring CmdLine;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    bool Result = true;

    if (Filename == L">")
    {
        CmdLine = Params;
    }
    else
    {
        CmdLine = L"\"" + Filename + L"\"";
        if (Params != L"")
        {
            CmdLine = CmdLine + L" " + Params;
        }

        if (WorkingDir == L"")
        {
            WorkingDir = extractFileDir(Filename);
        }
    }

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = ShowCmd;
    if (WorkingDir == L"")
    {
        // Obtain location of system directory eg C:\Windows\System32
        TCHAR systemDirPath[MAX_PATH];
        GetSystemDirectory(systemDirPath, sizeof(systemDirPath) / sizeof(TCHAR));
        WorkingDir = systemDirPath;
    }


    Result = CreateProcess(nullptr, const_cast<wchar_t*>(CmdLine.c_str()),
        nullptr, nullptr,
        false, CREATE_DEFAULT_ERROR_MODE, nullptr,
        WorkingDir.c_str(), &StartupInfo, &ProcessInfo);



    if (Result == false)
    {
        ResultCode = 0;
        return Result;
    }

    //  { Don't need the thread handle, so close it now }
    CloseHandle(ProcessInfo.hThread);
    HandleProcessWait(ProcessInfo.hProcess, Wait, ResultCode);

    return Result;
}

wstring Process::extractFileDir(wstring path)
{
    size_t pos = path.find_last_of(L"\\/");
    wstring dir_str = (wstring::npos == pos) ? L"" : path.substr(0, pos);
    return dir_str;
}

optional<DWORD> Process::InstExec(const wstring& appName, const wstring& commandLine,
                                  DWORD timeoutMS, WORD showWindowFlags)
{
    wostringstream exec;
    if (!appName.empty()) {
        exec << L"\"" << appName << L"\"";
    }

    if (!commandLine.empty()) {
        if (!exec.str().empty()) {
            exec << L" ";
        }
        exec << commandLine;
    }

    PROCESS_INFORMATION pi;
    ::ZeroMemory(&pi, sizeof(pi));

    auto exitGuard = wsl::wsScopeGuard([&]
        {
            if (pi.hProcess != NULL) {
                ::CloseHandle(pi.hProcess);
            }
        });

    STARTUPINFO si;
    ::ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = showWindowFlags;

    BOOL result = ::CreateProcess(nullptr, exec.str().data(), nullptr, nullptr, false,
        CREATE_DEFAULT_ERROR_MODE, nullptr, nullptr, &si, &pi);
    if (result == FALSE) {
        Log::instance().out("Process::InstExec CreateProcess(%ls) failed (%lu)", exec.str().c_str(), ::GetLastError());
        return std::nullopt;
    }

    ::CloseHandle(pi.hThread);

    if (timeoutMS == 0) {
        return NO_ERROR;
    }

    ::WaitForInputIdle(pi.hProcess, timeoutMS);

    DWORD waitResult = ::WaitForSingleObject(pi.hProcess, timeoutMS);

    if (waitResult == WAIT_FAILED) {
        Log::instance().out("Process::InstExec WaitForSingleObject failed (%lu)", ::GetLastError());
        return std::nullopt;
    }

    if (waitResult == WAIT_TIMEOUT) {
        ::TerminateProcess(pi.hProcess, 0);
        return WAIT_TIMEOUT;
    }

    DWORD processExitCode;
    result = ::GetExitCodeProcess(pi.hProcess, &processExitCode);
    if (result == FALSE) {
        Log::instance().out("Process::InstExec GetExitCodeProcess failed (%lu)", ::GetLastError());
        return std::nullopt;
    }

    return processExitCode;
}
