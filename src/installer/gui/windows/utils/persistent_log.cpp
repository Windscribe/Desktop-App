#include "persistent_log.h"

#include <aclapi.h>
#include <shlobj.h>

#include "wsscopeguard.h"

namespace wsl {

namespace {

void debugOut(const std::wstring &msg)
{
    ::OutputDebugStringW((L"PersistentLog: " + msg).c_str());
}

// Builds the protected DACL used for the log directory: Administrators + SYSTEM full
// access, Users read/list only.  On success the caller owns *acl and must LocalFree it.
DWORD buildLogDirectoryAcl(PACL *acl)
{
    BYTE sidAdmins[SECURITY_MAX_SID_SIZE];
    DWORD cbSid = sizeof(sidAdmins);
    if (!::CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, sidAdmins, &cbSid)) {
        return ::GetLastError();
    }

    BYTE sidSystem[SECURITY_MAX_SID_SIZE];
    cbSid = sizeof(sidSystem);
    if (!::CreateWellKnownSid(WinLocalSystemSid, nullptr, sidSystem, &cbSid)) {
        return ::GetLastError();
    }

    BYTE sidUsers[SECURITY_MAX_SID_SIZE];
    cbSid = sizeof(sidUsers);
    if (!::CreateWellKnownSid(WinBuiltinUsersSid, nullptr, sidUsers, &cbSid)) {
        return ::GetLastError();
    }

    EXPLICIT_ACCESS ea[3];
    ::ZeroMemory(&ea, sizeof(ea));

    ea[0].grfAccessPermissions = GENERIC_ALL;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[0].Trustee.ptstrName = reinterpret_cast<LPTSTR>(sidAdmins);

    ea[1].grfAccessPermissions = GENERIC_ALL;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_USER;
    ea[1].Trustee.ptstrName = reinterpret_cast<LPTSTR>(sidSystem);

    // Read-only for Users so they can retrieve the log for support.
    ea[2].grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
    ea[2].grfAccessMode = SET_ACCESS;
    ea[2].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[2].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[2].Trustee.ptstrName = reinterpret_cast<LPTSTR>(sidUsers);

    // SetEntriesInAcl copies the SIDs into the ACL it allocates, so the stack buffers
    // above do not need to outlive this call.
    return ::SetEntriesInAcl(3, ea, nullptr, acl);
}

// Creates the directory with a protected DACL: Administrators + SYSTEM full access,
// Users read/list only.  Returns a Win32 error code (ERROR_ALREADY_EXISTS if the
// directory was already there; its safety is verified separately).
DWORD createLogDirectory(const std::wstring &path)
{
    PACL pACL = nullptr;
    auto freeAcl = wsl::wsScopeGuard([&] {
        if (pACL) {
            ::LocalFree(pACL);
        }
    });

    DWORD result = buildLogDirectoryAcl(&pACL);
    if (result != ERROR_SUCCESS) {
        return result;
    }

    SECURITY_DESCRIPTOR sd;
    if (!::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) ||
        !::SetSecurityDescriptorDacl(&sd, TRUE, pACL, FALSE) ||
        !::SetSecurityDescriptorControl(&sd, SE_DACL_PROTECTED, SE_DACL_PROTECTED)) {
        return ::GetLastError();
    }

    SECURITY_ATTRIBUTES sa;
    ::ZeroMemory(&sa, sizeof(sa));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    if (!::CreateDirectoryW(path.c_str(), &sa)) {
        return ::GetLastError();
    }

    return ERROR_SUCCESS;
}

// Replaces the DACL of an existing directory with the same protected DACL that
// createLogDirectory applies to a new one.  A pre-existing directory may carry a weak
// DACL inherited from %ProgramData% (Users may create files there and keep full control
// of them via CREATOR OWNER), which would let a regular user tamper with the log or, if
// the DACL also granted DELETE, swap the directory while an elevated installer writes
// through it.  Only called after the owner check accepted the directory, so the DACL is
// not being "fixed" on an attacker-owned directory (whose owner could just rewrite it).
DWORD resetLogDirectoryAcl(const std::wstring &path)
{
    PACL pACL = nullptr;
    auto freeAcl = wsl::wsScopeGuard([&] {
        if (pACL) {
            ::LocalFree(pACL);
        }
    });

    DWORD result = buildLogDirectoryAcl(&pACL);
    if (result != ERROR_SUCCESS) {
        return result;
    }

    // PROTECTED_DACL_SECURITY_INFORMATION cuts inheritance from %ProgramData%, same as
    // SE_DACL_PROTECTED in createLogDirectory.
    return ::SetNamedSecurityInfoW(const_cast<LPWSTR>(path.c_str()), SE_FILE_OBJECT,
                                   DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                                   nullptr, nullptr, pACL, nullptr);
}

// A regular user can pre-create subdirectories in %ProgramData%.  Since this process is
// elevated, writing through an attacker-controlled directory (junction/symlink, or a DACL
// letting the attacker swap files for links) would be a privilege escalation vector.
// Accept the directory only if it is a real directory (not a reparse point) owned by
// Administrators or SYSTEM.
bool isSafeLogDirectory(const std::wstring &path)
{
    const DWORD attrs = ::GetFileAttributesW(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        debugOut(L"GetFileAttributes failed on " + path + L" (" + std::to_wstring(::GetLastError()) + L")");
        return false;
    }
    if (!(attrs & FILE_ATTRIBUTE_DIRECTORY) || (attrs & FILE_ATTRIBUTE_REPARSE_POINT)) {
        debugOut(L"not a real directory: " + path);
        return false;
    }

    PSID owner = nullptr;
    PSECURITY_DESCRIPTOR sd = nullptr;
    const DWORD result = ::GetNamedSecurityInfoW(path.c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
                                                 &owner, nullptr, nullptr, nullptr, &sd);
    if (result != ERROR_SUCCESS) {
        debugOut(L"GetNamedSecurityInfo failed on " + path + L" (" + std::to_wstring(result) + L")");
        return false;
    }
    auto freeSd = wsl::wsScopeGuard([&] {
        if (sd) {
            ::LocalFree(sd);
        }
    });

    BYTE sidAdmins[SECURITY_MAX_SID_SIZE];
    DWORD cbSid = sizeof(sidAdmins);
    if (::CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, sidAdmins, &cbSid) && ::EqualSid(owner, sidAdmins)) {
        return true;
    }

    BYTE sidSystem[SECURITY_MAX_SID_SIZE];
    cbSid = sizeof(sidSystem);
    if (::CreateWellKnownSid(WinLocalSystemSid, nullptr, sidSystem, &cbSid) && ::EqualSid(owner, sidSystem)) {
        return true;
    }

    debugOut(L"directory has an untrusted owner: " + path);
    return false;
}

}  // namespace

PersistentLog::~PersistentLog()
{
    close();
}

std::wstring PersistentLog::defaultDirectory()
{
    PWSTR programData = nullptr;
    HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_ProgramData, KF_FLAG_DEFAULT, nullptr, &programData);
    // Per the SHGetKnownFolderPath docs, the output buffer must be freed even on failure.
    if (FAILED(hr)) {
        ::CoTaskMemFree(programData);
        debugOut(L"SHGetKnownFolderPath(ProgramData) failed (" + std::to_wstring(hr) + L")");
        return std::wstring();
    }
    std::wstring path(programData);
    ::CoTaskMemFree(programData);
    return path + L"\\Windscribe";
}

std::wstring PersistentLog::defaultPath(const std::wstring &fileName)
{
    const std::wstring dir = defaultDirectory();
    if (dir.empty()) {
        return std::wstring();
    }
    return dir + L"\\" + fileName;
}

bool PersistentLog::open(const std::wstring &fileName, OpenMode mode)
{
    close();

    const std::wstring logFile = defaultPath(fileName);
    if (logFile.empty()) {
        return false;
    }
    const std::wstring logDir = logFile.substr(0, logFile.find_last_of(L'\\'));

    const DWORD err = createLogDirectory(logDir);
    if (err != ERROR_SUCCESS && err != ERROR_ALREADY_EXISTS) {
        debugOut(L"failed to create " + logDir + L" (" + std::to_wstring(err) + L")");
        return false;
    }

    if (!isSafeLogDirectory(logDir)) {
        return false;
    }

    if (err == ERROR_ALREADY_EXISTS) {
        // The directory passed the owner check but was not created by us just now, so its
        // DACL is not trustworthy; replace it with our protected one.  Fail closed: if the
        // DACL cannot be enforced, writing an elevated process's log there is not safe.
        const DWORD aclErr = resetLogDirectoryAcl(logDir);
        if (aclErr != ERROR_SUCCESS) {
            debugOut(L"failed to reset the DACL on " + logDir + L" (" + std::to_wstring(aclErr) + L")");
            return false;
        }
    }

    if (mode == OpenMode::Rotate) {
        // Keep the previous attempt's log around as .1.  If rotation fails, try to delete
        // the stale file instead.  If that fails too, fail closed: a file that can be
        // neither renamed nor deleted is not under our control (e.g. an attacker holding
        // it open without delete sharing), and appending an elevated process's log to it
        // is not safe.
        if (::GetFileAttributesW(logFile.c_str()) != INVALID_FILE_ATTRIBUTES) {
            if (!::MoveFileExW(logFile.c_str(), (logFile + L".1").c_str(), MOVEFILE_REPLACE_EXISTING)) {
                debugOut(L"failed to rotate " + logFile + L" (" + std::to_wstring(::GetLastError()) + L")");
                if (!::DeleteFileW(logFile.c_str()) && ::GetLastError() != ERROR_FILE_NOT_FOUND) {
                    debugOut(L"failed to delete " + logFile + L" (" + std::to_wstring(::GetLastError()) + L")");
                    return false;
                }
            }
        }
    }

    // FILE_FLAG_OPEN_REPARSE_POINT ensures we never write through a symlink planted in
    // the directory; the handle check below rejects the reparse point itself.
    // Append-only access (rather than GENERIC_WRITE) plus write sharing allows two
    // cooperating processes to interleave entries safely: the uninstaller's first phase
    // may still hold the file open while the second phase it spawned starts logging.
    // FILE_SHARE_DELETE lets the uninstaller's second phase delete the log directory
    // even if the first phase still holds its handle to uninstaller.log.
    HANDLE handle = ::CreateFileW(logFile.c_str(), FILE_APPEND_DATA | FILE_READ_ATTRIBUTES,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        debugOut(L"failed to open " + logFile + L" (" + std::to_wstring(::GetLastError()) + L")");
        return false;
    }

    // Reject reparse points (symlinks) and hardlinks: a hardlink is not a reparse point,
    // so it would pass the attribute check, yet appending through it would write into
    // some other file elsewhere on the volume.
    BY_HANDLE_FILE_INFORMATION info;
    if (!::GetFileInformationByHandle(handle, &info) || (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
        info.nNumberOfLinks > 1) {
        debugOut(L"log file is, or could not be verified as, a regular file: " + logFile);
        ::CloseHandle(handle);
        return false;
    }

    handle_ = handle;
    path_ = logFile;
    return true;
}

void PersistentLog::close()
{
    if (handle_ != INVALID_HANDLE_VALUE) {
        ::CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
}

void PersistentLog::write(const std::string &utf8)
{
    if (handle_ == INVALID_HANDLE_VALUE || utf8.empty()) {
        return;
    }

    DWORD written = 0;
    ::WriteFile(handle_, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
}

}
