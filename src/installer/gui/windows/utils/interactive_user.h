#pragma once

#include <Windows.h>

#include <string>
#include <vector>

#include "win32handle.h"

namespace wsl {

// The user who owns the desktop shell (Explorer) in the current session: the person sitting at
// the computer.
//
// The installer and uninstaller run elevated, which causes two problems when they touch per-user
// data:
// - The elevated process may run as a different account.  When a standard user enters an
//   administrator's credentials at the UAC prompt, the process's HKCU and known folders belong to
//   the administrator, not to the person using the computer.
// - Even as the same account, the process has full administrator rights, while the user's profile
//   is writable by the user.  Malware running as the user can plant a junction, or redirect a
//   known folder, so that an elevated delete lands on a protected location.
// Per-user data should therefore be modified while impersonating this user (see
// ScopedImpersonation): that targets the right profile, and every file and registry operation is
// then limited to the rights the user already has.
class InteractiveUser
{
public:
    // Obtains an impersonation token for the shell's user.  Returns ERROR_SUCCESS, or a Win32
    // error code: ERROR_NOT_FOUND when no shell runs in this session (e.g. a silent uninstall run
    // as SYSTEM by a management tool), or the error from opening or duplicating the shell's token.
    DWORD open();

    // Valid only after open() succeeded.
    HANDLE token() const { return token_.getHandle(); }

    // The user's SID in string form, for logging.  Empty until open() succeeds.
    std::wstring sidString() const;

    // True if the user is the account the current process runs as.  False if open() has not
    // succeeded.
    bool isProcessUser() const;

private:
    Win32Handle token_;
    std::vector<BYTE> sid_;
};

// Impersonates the given token on the current thread for the lifetime of the object.
// HKEY_CURRENT_USER is cached per process, so it keeps referring to the process's own hive while
// impersonating: use RegOpenCurrentUser to reach the impersonated user's hive.  Known folder paths
// must likewise be resolved with SHGetKnownFolderPath and the user's token.
class ScopedImpersonation
{
public:
    explicit ScopedImpersonation(HANDLE token);
    ~ScopedImpersonation();

    ScopedImpersonation(const ScopedImpersonation &) = delete;
    ScopedImpersonation &operator=(const ScopedImpersonation &) = delete;

    bool isActive() const { return active_; }
    // ERROR_SUCCESS when active; otherwise why impersonation failed.
    DWORD error() const { return error_; }

private:
    bool active_ = false;
    DWORD error_ = ERROR_SUCCESS;
};

}
