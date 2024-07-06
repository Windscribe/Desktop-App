#include "utils.h"

#include <Windows.h>
#include <Shlobj.h>

#include <filesystem>
#include <sstream>

#include "applicationinfo.h"
#include "logger.h"
#include "path.h"
#include "win32handle.h"
#include "wsscopeguard.h"

using namespace std;

namespace Utils
{

wstring getSystemDir()
{
    wchar_t path[MAX_PATH];
    UINT result = ::GetSystemDirectory(path, MAX_PATH);
    if (result == 0 || result >= MAX_PATH) {
        Log::instance().out(L"GetSystemDir failed (%lu)", ::GetLastError());
        return wstring();
    }

    return wstring(path);
}

optional<DWORD> instExec(const wstring& appName, const wstring& commandLine, DWORD timeoutMS,
                         WORD showWindowFlags, const wstring &currentFolder, DWORD *lastError)
{
    wostringstream stream;
    if (!appName.empty()) {
        stream << L"\"" << appName << L"\"";
    }

    if (!commandLine.empty()) {
        if (!stream.str().empty()) {
            stream << L" ";
        }
        stream << commandLine;
    }

    PROCESS_INFORMATION pi;
    ::ZeroMemory(&pi, sizeof(pi));

    auto exitGuard = wsl::wsScopeGuard([&] {
        if (pi.hProcess != NULL) {
            ::CloseHandle(pi.hProcess);
        }
    });

    // As per the Win32 docs; the Unicode version of CreateProcess 'may' modify its lpCommandLine
    // parameter.  Therefore, this parameter cannot be a pointer to read-only memory (such as a
    // const variable or a literal string).  If this parameter is a constant string, CreateProcess
    // may cause an access violation.  Maximum length of the lpCommandLine parameter is 32767.
    unique_ptr<wchar_t[]> exec(new wchar_t[32767]);
    wcsncpy_s(exec.get(), 32767, stream.str().c_str(), _TRUNCATE);

    STARTUPINFO si;
    ::ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = showWindowFlags;

    LPCWSTR lpCurrentDirectory = nullptr;
    if (!currentFolder.empty()) {
        lpCurrentDirectory = currentFolder.c_str();
    }

    BOOL result = ::CreateProcess(nullptr, exec.get(), nullptr, nullptr, false, CREATE_DEFAULT_ERROR_MODE,
                                  nullptr, lpCurrentDirectory, &si, &pi);
    if (result == FALSE) {
        Log::instance().out(L"Process::InstExec CreateProcess(%s) failed (%lu)", exec.get(), ::GetLastError());
        if (lastError) {
            *lastError = ::GetLastError();
        }
        return nullopt;
    }

    ::CloseHandle(pi.hThread);

    if (timeoutMS == 0) {
        return NO_ERROR;
    }

    DWORD waitResult = ::WaitForInputIdle(pi.hProcess, timeoutMS);
    if (waitResult == WAIT_FAILED) {
        if (::GetLastError() != ERROR_NOT_GUI_PROCESS) {
            // We're not treating this error as critical, but still want to note it.
            Log::instance().out(L"Process::InstExec WaitForInputIdle failed (%lu)", ::GetLastError());
        }
    }

    waitResult = ::WaitForSingleObject(pi.hProcess, timeoutMS);

    if (waitResult == WAIT_FAILED) {
        Log::instance().out(L"Process::InstExec WaitForSingleObject failed (%lu)", ::GetLastError());
        if (lastError) {
            *lastError = ::GetLastError();
        }
        return nullopt;
    }

    if (waitResult == WAIT_TIMEOUT) {
        ::TerminateProcess(pi.hProcess, 0);
        return WAIT_TIMEOUT;
    }

    DWORD processExitCode;
    result = ::GetExitCodeProcess(pi.hProcess, &processExitCode);
    if (result == FALSE) {
        Log::instance().out(L"Process::InstExec GetExitCodeProcess failed (%lu)", ::GetLastError());
        if (lastError) {
            *lastError = ::GetLastError();
        }
        return nullopt;
    }

    return processExitCode;
}

wstring desktopFolder()
{
    wchar_t szPath[MAX_PATH];
    HRESULT hResult = ::SHGetFolderPath(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL,
                                        SHGFP_TYPE_CURRENT, szPath);
    if (FAILED(hResult)) {
        Log::instance().out(L"SHGetFolderPath(CSIDL_DESKTOPDIRECTORY) failed (%lu)", ::GetLastError());
        return wstring();
    }

    return wstring(szPath);
}

wstring startMenuProgramsFolder()
{
    wchar_t szPath[MAX_PATH];
    HRESULT hResult = ::SHGetFolderPath(NULL, CSIDL_COMMON_PROGRAMS, NULL,
                                        SHGFP_TYPE_CURRENT, szPath);
    if (FAILED(hResult)) {
        Log::instance().out(L"SHGetFolderPath(CSIDL_COMMON_PROGRAMS) failed (%lu)", ::GetLastError());
        return wstring();
    }

    return wstring(szPath);
}

class EnumWindowInfo
{
public:
    HWND appMainWindow = NULL;
};

static BOOL CALLBACK
FindAppWindowHandleProc(HWND hwnd, LPARAM lParam)
{
    DWORD processID = 0;
    ::GetWindowThreadProcessId(hwnd, &processID);

    if (processID == 0) {
        Log::instance().out(L"FindAppWindowHandleProc GetWindowThreadProcessId failed %lu", ::GetLastError());
        return TRUE;
    }

    wsl::Win32Handle hProcess(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID));
    if (!hProcess.isValid())
    {
        if (::GetLastError() != ERROR_ACCESS_DENIED) {
            Log::instance().out(L"FindAppWindowHandleProc OpenProcess failed %lu", ::GetLastError());
        }
        return TRUE;
    }

    TCHAR imageName[MAX_PATH];
    DWORD pathLen = sizeof(imageName) / sizeof(imageName[0]);
    BOOL result = ::QueryFullProcessImageName(hProcess.getHandle(), 0, imageName, &pathLen);

    if (result == FALSE) {
        Log::instance().out(L"FindAppWindowHandleProc QueryFullProcessImageName failed %lu", ::GetLastError());
        return TRUE;
    }

    wstring exeName = Path::extractName(wstring(imageName, pathLen));

    if (_wcsicmp(exeName.c_str(), ApplicationInfo::appExeName().c_str()) == 0)
    {
        TCHAR buffer[128];
        int resultLen = ::GetWindowText(hwnd, buffer, sizeof(buffer) / sizeof(buffer[0]));

        if (resultLen > 0 && (_wcsicmp(buffer, ApplicationInfo::name().c_str()) == 0))
        {
            EnumWindowInfo* pWindowInfo = (EnumWindowInfo*)lParam;
            pWindowInfo->appMainWindow = hwnd;
            return FALSE;
        }
    }

    return TRUE;
}

HWND appMainWindowHandle()
{
    auto pWindowInfo = make_unique<EnumWindowInfo>();
    ::EnumWindows((WNDENUMPROC)FindAppWindowHandleProc, (LPARAM)pWindowInfo.get());

    return pWindowInfo->appMainWindow;
}

wstring getExePath()
{
    wchar_t path[MAX_PATH];
    int ret = GetModuleFileName(NULL, path, MAX_PATH);
    if (ret == 0) {
        return wstring();
    }
    return filesystem::path(path).parent_path().wstring();
}

wstring programFilesFolder()
{
    TCHAR programFilesPath[MAX_PATH];
    BOOL result = ::SHGetSpecialFolderPath(0, programFilesPath, CSIDL_PROGRAM_FILES, FALSE);
    if (!result) {
        Log::WSDebugMessage(L"programFilesFolder - SHGetSpecialFolderPath failed, using default");
        return wstring(L"C:\\Program Files");
    }

    return wstring(programFilesPath);
}

}
