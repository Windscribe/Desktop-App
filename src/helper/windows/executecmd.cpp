#include "executecmd.h"

#include <optional>

#include <spdlog/spdlog.h>
#include "utils.h"
#include "utils/win32handle.h"

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

ExecuteCmdResult ExecuteCmd::executeNonblockingCmd(const std::wstring &cmd, const std::wstring &workingDir)
{
    ExecuteCmdResult res;

    // As per the Win32 docs; the Unicode version of CreateProcess 'may' modify its lpCommandLine
    // parameter.  Therefore, this parameter cannot be a pointer to read-only memory (such as a
    // const variable or a literal string).  If this parameter is a constant string, CreateProcess
    // may cause an access violation.  Maximum length of the lpCommandLine parameter is 32767.
    std::unique_ptr<wchar_t[]> exec(new wchar_t[32767]);
    wcsncpy_s(exec.get(), 32767, cmd.c_str(), _TRUNCATE);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = NULL;
    si.hStdError = NULL;
    si.hStdOutput = NULL;

    ZeroMemory( &pi, sizeof(pi) );
    if (CreateProcess(NULL, exec.get(), NULL, NULL, FALSE, CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS, NULL, workingDir.c_str(), &si, &pi))
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        res.success = true;
    }
    else
    {
        spdlog::error("executeNonblockingCmd CreateProcess failed: {}", GetLastError());
    }

    return res;
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
