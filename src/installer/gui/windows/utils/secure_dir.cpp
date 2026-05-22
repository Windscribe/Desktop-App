#include "secure_dir.h"

#include <aclapi.h>
#include <bcrypt.h>

#include <array>

#include "wsscopeguard.h"

namespace wsl {

namespace {

void emit(const std::function<void(const std::wstring&)> &logFunc, const std::wstring &msg)
{
    if (logFunc) {
        logFunc(msg);
    }
}

}

std::optional<std::wstring> generateRandomSuffix()
{
    std::array<unsigned char, 16> buf{};
    NTSTATUS status = ::BCryptGenRandom(nullptr, buf.data(), static_cast<ULONG>(buf.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (!BCRYPT_SUCCESS(status)) {
        return std::nullopt;
    }

    static constexpr wchar_t hexChars[] = L"0123456789abcdef";
    std::wstring result;
    result.reserve(buf.size() * 2);
    for (unsigned char b : buf) {
        result.push_back(hexChars[(b >> 4) & 0xF]);
        result.push_back(hexChars[b & 0xF]);
    }
    return result;
}

DWORD createAdminOnlyDirectory(const std::filesystem::path &path,
                               const std::function<void(const std::wstring&)> &logFunc)
{
    SECURITY_DESCRIPTOR sd;
    ::ZeroMemory(&sd, sizeof(sd));

    if (!::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        DWORD err = ::GetLastError();
        emit(logFunc, L"createAdminOnlyDirectory InitializeSecurityDescriptor failed: " + std::to_wstring(err));
        return err;
    }

    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    PSID sidAdmin = nullptr;
    if (!::AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sidAdmin)) {
        DWORD err = ::GetLastError();
        emit(logFunc, L"createAdminOnlyDirectory AllocateAndInitializeSid(Admins) failed: " + std::to_wstring(err));
        return err;
    }
    auto freeAdmin = wsl::wsScopeGuard([&] {
        if (sidAdmin) {
            ::FreeSid(sidAdmin);
        }
    });

    PSID sidSystem = nullptr;
    if (!::AllocateAndInitializeSid(&SIDAuthNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &sidSystem)) {
        DWORD err = ::GetLastError();
        emit(logFunc, L"createAdminOnlyDirectory AllocateAndInitializeSid(System) failed: " + std::to_wstring(err));
        return err;
    }
    auto freeSystem = wsl::wsScopeGuard([&] {
        if (sidSystem) {
            ::FreeSid(sidSystem);
        }
    });

    EXPLICIT_ACCESS ea[2];
    ::ZeroMemory(&ea, sizeof(ea));

    ea[0].grfAccessPermissions = GENERIC_ALL;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[0].Trustee.ptstrName = reinterpret_cast<LPTSTR>(sidAdmin);

    // SYSTEM is included so that anything running in the LocalSystem context (e.g. Windscribe's own
    // service during an upgrade, Defender/AV, and the Session Manager processing pending file/folder
    // deletes for MOVEFILE_DELAY_UNTIL_REBOOT at boot) can access the folder without needing
    // SeBackup/SeRestore.  SYSTEM is already equivalent to admin in the threat model.
    ea[1].grfAccessPermissions = GENERIC_ALL;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_USER;
    ea[1].Trustee.ptstrName = reinterpret_cast<LPTSTR>(sidSystem);

    PACL pACL = nullptr;
    auto freeAcl = wsl::wsScopeGuard([&] {
        if (pACL) {
            ::LocalFree(pACL);
        }
    });

    DWORD result = ::SetEntriesInAcl(2, ea, nullptr, &pACL);
    if (result != ERROR_SUCCESS) {
        emit(logFunc, L"createAdminOnlyDirectory SetEntriesInAcl failed: " + std::to_wstring(result));
        return result;
    }

    if (!::SetSecurityDescriptorDacl(&sd, TRUE, pACL, FALSE)) {
        DWORD err = ::GetLastError();
        emit(logFunc, L"createAdminOnlyDirectory SetSecurityDescriptorDacl failed: " + std::to_wstring(err));
        return err;
    }

    // Block inheritance from the parent directory (e.g. C:\Windows\Temp) so the DACL is exactly what we specified.
    if (!::SetSecurityDescriptorControl(&sd, SE_DACL_PROTECTED, SE_DACL_PROTECTED)) {
        DWORD err = ::GetLastError();
        emit(logFunc, L"createAdminOnlyDirectory SetSecurityDescriptorControl failed: " + std::to_wstring(err));
        return err;
    }

    SECURITY_ATTRIBUTES sa;
    ::ZeroMemory(&sa, sizeof(sa));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    if (!::CreateDirectoryW(path.c_str(), &sa)) {
        DWORD err = ::GetLastError();
        emit(logFunc, L"createAdminOnlyDirectory CreateDirectoryW failed: " + std::to_wstring(err));
        return err;
    }

    return ERROR_SUCCESS;
}

}
