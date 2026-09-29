#include "executecmd.h"

#include <optional>

#include <spdlog/spdlog.h>
#include "utils.h"
#include "utils/systemlibloader.h"
#include "utils/win32handle.h"
#include "utils/wsscopeguard.h"

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

ExecuteCmdResult ExecuteCmd::executeNonblockingCmd(const std::wstring &cmd, const std::wstring &workingDir, HANDLE *outProcessHandle)
{
    ExecuteCmdResult res;
    if (outProcessHandle) {
        *outProcessHandle = NULL;
    }

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
        res.processId = pi.dwProcessId;
        if (outProcessHandle) {
            // Transfer ownership of the process handle to the caller so the PID cannot be reused
            // while the caller holds it open.
            *outProcessHandle = pi.hProcess;
        } else {
            CloseHandle(pi.hProcess);
        }
        res.success = true;
    }
    else
    {
        spdlog::error("executeNonblockingCmd CreateProcess failed: {}", GetLastError());
    }

    return res;
}

ExecuteCmdResult ExecuteCmd::executeNonblockingCmdAsUser(const std::wstring &exePath, const std::wstring &cmdLine,
                                                         const std::wstring &workingDir, HANDLE userToken,
                                                         HANDLE inheritHandle, HANDLE *outProcessHandle)
{
    ExecuteCmdResult res;
    if (outProcessHandle) {
        *outProcessHandle = NULL;
    }

    if (userToken == nullptr || userToken == INVALID_HANDLE_VALUE) {
        spdlog::error("executeNonblockingCmdAsUser: missing user token");
        return res;
    }
    if (exePath.empty() || cmdLine.empty()) {
        spdlog::error("executeNonblockingCmdAsUser: empty exe or command line");
        return res;
    }

    std::unique_ptr<wchar_t[]> exec(new wchar_t[32767]);
    wcsncpy_s(exec.get(), 32767, cmdLine.c_str(), _TRUNCATE);

    const bool inherit = (inheritHandle != nullptr && inheritHandle != INVALID_HANDLE_VALUE);
    const DWORD attrCount = inherit ? 1 : 0;

    SIZE_T attrSize = 0;
    ::InitializeProcThreadAttributeList(nullptr, attrCount, 0, &attrSize);
    if (attrSize == 0) {
        spdlog::error("executeNonblockingCmdAsUser InitializeProcThreadAttributeList(size) failed: {}", ::GetLastError());
        return res;
    }

    auto *attrList = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(::HeapAlloc(::GetProcessHeap(), 0, attrSize));
    if (attrList == nullptr) {
        spdlog::error("executeNonblockingCmdAsUser HeapAlloc failed");
        return res;
    }
    auto freeAttr = wsl::wsScopeGuard([&] {
        ::DeleteProcThreadAttributeList(attrList);
        ::HeapFree(::GetProcessHeap(), 0, attrList);
    });

    if (!::InitializeProcThreadAttributeList(attrList, attrCount, 0, &attrSize)) {
        spdlog::error("executeNonblockingCmdAsUser InitializeProcThreadAttributeList failed: {}", ::GetLastError());
        freeAttr.dismiss();
        ::HeapFree(::GetProcessHeap(), 0, attrList);
        return res;
    }

    HANDLE inheritHandles[1] = { inheritHandle };
    if (inherit) {
        if (!::UpdateProcThreadAttribute(attrList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                         inheritHandles, sizeof(HANDLE), nullptr, nullptr)) {
            spdlog::error("executeNonblockingCmdAsUser UpdateProcThreadAttribute failed: {}", ::GetLastError());
            return res;
        }
    }

    STARTUPINFOEXW siex;
    ZeroMemory(&siex, sizeof(siex));
    siex.StartupInfo.cb = sizeof(siex);
    siex.lpAttributeList = attrList;

    PVOID envBlock = nullptr;
    bool haveEnv = false;
    try {
        // This DLL is not in the KnownDLLs list.
        wsl::SystemLibLoader userenvLib("userenv.dll");
        const auto createEnvironmentBlock = userenvLib.getFunction<BOOL WINAPI(LPVOID *, HANDLE, BOOL)>("CreateEnvironmentBlock");
        const auto destroyEnvironmentBlock = userenvLib.getFunction<BOOL WINAPI(LPVOID)>("DestroyEnvironmentBlock");
        haveEnv = createEnvironmentBlock(&envBlock, userToken, FALSE) != FALSE;
        if (!haveEnv) {
            spdlog::warn("executeNonblockingCmdAsUser CreateEnvironmentBlock failed ({}); launching without a user env block",
                         ::GetLastError());
        }
        auto envGuard = wsl::wsScopeGuard([&] {
            if (haveEnv) {
                destroyEnvironmentBlock(envBlock);
            }
        });

        const DWORD flags = CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT | EXTENDED_STARTUPINFO_PRESENT;
        const wchar_t *cwd = workingDir.empty() ? nullptr : workingDir.c_str();

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));

        BOOL created = ::CreateProcessAsUserW(userToken, exePath.c_str(), exec.get(), nullptr, nullptr,
                                              inherit ? TRUE : FALSE, flags, haveEnv ? envBlock : nullptr, cwd,
                                              &siex.StartupInfo, &pi);
        if (!created) {
            spdlog::error("executeNonblockingCmdAsUser CreateProcessAsUser failed: {}", ::GetLastError());
            return res;
        }

        ::CloseHandle(pi.hThread);
        res.processId = pi.dwProcessId;
        if (outProcessHandle) {
            *outProcessHandle = pi.hProcess;
        } else {
            ::CloseHandle(pi.hProcess);
        }
        res.success = true;
        return res;
    }
    catch (const std::system_error &ex) {
        spdlog::error("executeNonblockingCmdAsUser userenv load failed: {}", ex.what());
        return res;
    }
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
