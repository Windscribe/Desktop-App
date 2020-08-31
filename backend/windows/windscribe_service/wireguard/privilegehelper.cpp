#include "../all_headers.h"
#include "privilegehelper.h"
#include "../logger.h"

PrivilegeHelper::PrivilegeHelper() : is_elevated_(false)
{
    TOKEN_PRIVILEGES privileges;
    memset(&privileges, 0, sizeof(privileges));
    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &privileges.Privileges[0].Luid)) {
        Logger::instance().out(L"LookupPrivilegeValue() failed");
        return;
    }
    auto process_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (process_snapshot == INVALID_HANDLE_VALUE) {
        Logger::instance().out(L"CreateToolhelp32Snapshot() failed");
        return;
    }
    const auto kWinLogonName(L"winlogon.exe");
    PROCESSENTRY32 entry;
    memset(&entry, 0, sizeof(entry));
    entry.dwSize = sizeof(PROCESSENTRY32);
    for (auto ret = Process32First(process_snapshot, &entry); ret;
        ret = Process32Next(process_snapshot, &entry)) {
        if (_wcsicmp(kWinLogonName, entry.szExeFile))
            continue;
        RevertToSelf();
        if (!ImpersonateSelf(SecurityImpersonation))
            continue;
        HANDLE thread_token;
        if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES, FALSE, &thread_token))
            continue;
        if (!AdjustTokenPrivileges(thread_token, FALSE, &privileges, sizeof(privileges),
            nullptr, nullptr)) {
            Logger::instance().out(L"AdjustTokenPrivileges() failed");
            CloseHandle(thread_token);
            continue;
        }
        CloseHandle(thread_token);
        auto winlogon_process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, entry.th32ProcessID);
        if (!winlogon_process)
            winlogon_process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                entry.th32ProcessID);
        if (!winlogon_process)
            continue;
        HANDLE winlogon_token, duplicated_token;
        if (!OpenProcessToken(winlogon_process, TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
            &winlogon_token))
            continue;
        CloseHandle(winlogon_process);
        if (!DuplicateToken(winlogon_token, SecurityImpersonation, &duplicated_token)) {
            Logger::instance().out(L"DuplicateToken() failed");
            RevertToSelf();
            continue;
        }
        CloseHandle(winlogon_token);
        if (!SetThreadToken(nullptr, duplicated_token)) {
            Logger::instance().out(L"SetThreadToken() failed");
            CloseHandle(duplicated_token);
            continue;
        }
        CloseHandle(duplicated_token);
        is_elevated_ = true;
        break;
    }
    CloseHandle(process_snapshot);
}

PrivilegeHelper::~PrivilegeHelper()
{
    if (is_elevated_)
        RevertToSelf();
}

bool PrivilegeHelper::checkElevation() const
{
    return is_elevated_;
}

bool PrivilegeHelper::checkElevationForHandle(HANDLE handle) const
{
    if (!checkElevation())
        return false;
    SID expected_sid;
    DWORD bytes = sizeof(expected_sid);
    if (!CreateWellKnownSid(WinLocalSystemSid, NULL, &expected_sid, &bytes)) {
        Logger::instance().out(L"CreateWellKnownSid() failed");
        return false;
    }
    PSECURITY_DESCRIPTOR handle_sd;
    PSID handle_sid;
    auto error = GetSecurityInfo(handle, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
        &handle_sid, NULL, NULL, NULL, &handle_sd);
    if (error != ERROR_SUCCESS) {
        Logger::instance().out(L"GetSecurityInfo() failed");
        return false;
    }
    error = EqualSid(&expected_sid, handle_sid) ? ERROR_SUCCESS : ERROR_ACCESS_DENIED;
    LocalFree(handle_sd);
    if (error != ERROR_SUCCESS)
        return false;
    return true;
}
