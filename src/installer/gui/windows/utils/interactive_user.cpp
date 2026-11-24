#include "interactive_user.h"

#include <sddl.h>

namespace wsl {

namespace {

DWORD tokenUserSid(HANDLE token, std::vector<BYTE> &sid)
{
    DWORD size = 0;
    if (!::GetTokenInformation(token, TokenUser, nullptr, 0, &size)) {
        const DWORD err = ::GetLastError();
        if (err != ERROR_INSUFFICIENT_BUFFER) {
            return err;
        }
    }

    std::vector<BYTE> buffer(size);
    if (!::GetTokenInformation(token, TokenUser, buffer.data(), size, &size)) {
        return ::GetLastError();
    }

    const PSID userSid = reinterpret_cast<const TOKEN_USER *>(buffer.data())->User.Sid;
    const BYTE *first = static_cast<const BYTE *>(userSid);
    sid.assign(first, first + ::GetLengthSid(userSid));
    return ERROR_SUCCESS;
}

}

DWORD InteractiveUser::open()
{
    token_.closeHandle();
    sid_.clear();

    // The shell window is per desktop, so this is the shell of the session we are running in.
    const HWND shellWindow = ::GetShellWindow();
    if (shellWindow == nullptr) {
        return ERROR_NOT_FOUND;
    }

    DWORD pid = 0;
    ::GetWindowThreadProcessId(shellWindow, &pid);
    if (pid == 0) {
        return ERROR_NOT_FOUND;
    }

    Win32Handle process(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
    if (!process.isValid()) {
        return ::GetLastError();
    }

    Win32Handle primaryToken;
    if (!::OpenProcessToken(process.getHandle(), TOKEN_QUERY | TOKEN_DUPLICATE, primaryToken.data())) {
        return ::GetLastError();
    }

    // SHGetKnownFolderPath needs TOKEN_QUERY and TOKEN_IMPERSONATE on the token it is given, and
    // "in some cases" TOKEN_DUPLICATE as well.
    Win32Handle impersonationToken;
    if (!::DuplicateTokenEx(primaryToken.getHandle(), TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE, nullptr,
                            SecurityImpersonation, TokenImpersonation, impersonationToken.data())) {
        return ::GetLastError();
    }

    std::vector<BYTE> sid;
    const DWORD err = tokenUserSid(impersonationToken.getHandle(), sid);
    if (err != ERROR_SUCCESS) {
        return err;
    }

    token_ = std::move(impersonationToken);
    sid_ = std::move(sid);
    return ERROR_SUCCESS;
}

std::wstring InteractiveUser::sidString() const
{
    if (sid_.empty()) {
        return std::wstring();
    }

    LPWSTR str = nullptr;
    if (!::ConvertSidToStringSidW(const_cast<BYTE *>(sid_.data()), &str)) {
        return std::wstring();
    }

    std::wstring result(str);
    ::LocalFree(str);
    return result;
}

bool InteractiveUser::isProcessUser() const
{
    if (sid_.empty()) {
        return false;
    }

    Win32Handle processToken;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, processToken.data())) {
        return false;
    }

    std::vector<BYTE> processSid;
    if (tokenUserSid(processToken.getHandle(), processSid) != ERROR_SUCCESS) {
        return false;
    }

    return ::EqualSid(processSid.data(), const_cast<BYTE *>(sid_.data())) != FALSE;
}

ScopedImpersonation::ScopedImpersonation(HANDLE token)
{
    if (!::ImpersonateLoggedOnUser(token)) {
        error_ = ::GetLastError();
        return;
    }

    // Without SeImpersonatePrivilege, Windows does not fail the call: it impersonates at
    // identification level, where every access check fails.  Report that as a failure so the
    // caller can log it, rather than having each operation fail with access denied.
    Win32Handle threadToken;
    SECURITY_IMPERSONATION_LEVEL level = SecurityAnonymous;
    DWORD size = sizeof(level);
    if (!::OpenThreadToken(::GetCurrentThread(), TOKEN_QUERY, TRUE, threadToken.data()) ||
        !::GetTokenInformation(threadToken.getHandle(), TokenImpersonationLevel, &level, sizeof(level), &size)) {
        error_ = ::GetLastError();
        ::RevertToSelf();
        return;
    }

    if (level < SecurityImpersonation) {
        error_ = ERROR_BAD_IMPERSONATION_LEVEL;
        ::RevertToSelf();
        return;
    }

    active_ = true;
}

ScopedImpersonation::~ScopedImpersonation()
{
    // If RevertToSelf failed, the thread would keep the user's rights, not gain any; later
    // privileged operations would fail, which is the safe direction.
    if (active_) {
        ::RevertToSelf();
    }
}

}
