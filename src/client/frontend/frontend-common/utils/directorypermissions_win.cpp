#include "directorypermissions.h"

#include <Windows.h>
#include <AclAPI.h>
#include <Authz.h>
#include <Lm.h>
#include <Shlwapi.h>
#include <string>
#include <utility>
#include <vector>

#include "utils/wsscopeguard.h"

namespace DirectoryPermissions
{

namespace {

const ACCESS_MASK kRiskyWriteMask =
    FILE_ADD_FILE | FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_DELETE_CHILD
    | GENERIC_WRITE | GENERIC_ALL | WRITE_DAC | WRITE_OWNER | DELETE;

std::vector<BYTE> copySid(PSID src)
{
    if (!src || !IsValidSid(src)) return {};
    const DWORD len = GetLengthSid(src);
    std::vector<BYTE> buf(len);
    if (!CopySid(len, buf.data(), src)) return {};
    return buf;
}

std::vector<BYTE> getCurrentUserSid()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return {};
    }
    DWORD size = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        CloseHandle(token);
        return {};
    }
    std::vector<BYTE> buf(size);
    if (!GetTokenInformation(token, TokenUser, buf.data(), size, &size)) {
        CloseHandle(token);
        return {};
    }
    CloseHandle(token);
    TOKEN_USER *tu = reinterpret_cast<TOKEN_USER *>(buf.data());
    return copySid(tu->User.Sid);
}

// NetLocalGroupGetMembers takes a group account name, which is localized on
// non-English Windows installs (e.g. "Administratoren" on German).  Resolve
// the well-known BUILTIN\Administrators SID to its localized name via a
// two-call size-query pattern.  Falls back to the English literal as a last
// resort so enumeration at least works on English systems even if the lookup
// misbehaves.
std::wstring localAdminsGroupName()
{
    DWORD sidSize = SECURITY_MAX_SID_SIZE;
    std::vector<BYTE> sidBuf(sidSize);
    PSID adminSid = sidBuf.data();
    if (!CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, adminSid, &sidSize)) {
        return L"Administrators";
    }

    DWORD nameLen = 0, domainLen = 0;
    SID_NAME_USE use = SidTypeUnknown;
    LookupAccountSidW(nullptr, adminSid, nullptr, &nameLen, nullptr, &domainLen, &use);
    if (nameLen == 0) {
        return L"Administrators";
    }

    std::wstring name(nameLen, L'\0');
    std::wstring domain(domainLen, L'\0');
    if (!LookupAccountSidW(nullptr, adminSid, name.data(), &nameLen,
                           domain.data(), &domainLen, &use)) {
        return L"Administrators";
    }
    name.resize(nameLen);  // LookupAccountSidW reports length excluding trailing NUL on success.
    return name;
}

std::vector<std::vector<BYTE>> enumerateLocalAdminMembers()
{
    std::vector<std::vector<BYTE>> result;
    const std::wstring group = localAdminsGroupName();
    if (group.empty()) return result;

    DWORD_PTR resume = 0;
    DWORD_PTR prevResume = static_cast<DWORD_PTR>(-1);
    for (;;) {
        LOCALGROUP_MEMBERS_INFO_2 *buf = nullptr;
        DWORD entriesRead = 0, totalEntries = 0;
        const NET_API_STATUS status = NetLocalGroupGetMembers(
            nullptr, group.c_str(), 2,
            reinterpret_cast<LPBYTE *>(&buf), MAX_PREFERRED_LENGTH,
            &entriesRead, &totalEntries, &resume);
        if ((status == NERR_Success || status == ERROR_MORE_DATA) && buf) {
            for (DWORD i = 0; i < entriesRead; ++i) {
                std::vector<BYTE> sid = copySid(buf[i].lgrmi2_sid);
                if (!sid.empty()) result.push_back(std::move(sid));
            }
            NetApiBufferFree(buf);
        }
        if (status != ERROR_MORE_DATA) break;
        // Defensive: if the resume handle didn't advance (shouldn't happen per docs,
        // but guards against a hang if a broken driver returns ERROR_MORE_DATA without
        // updating resume or populating buf), stop iterating.
        if (resume == prevResume) break;
        prevResume = resume;
    }
    return result;
}

// Current user + all members of the local Administrators group.  Cached for
// the process lifetime — Administrators membership rarely changes mid-session
// and the enumeration is the expensive part of this module.
const std::vector<std::vector<BYTE>> &trustedSids()
{
    static const std::vector<std::vector<BYTE>> cached = [] {
        std::vector<std::vector<BYTE>> v;
        std::vector<BYTE> me = getCurrentUserSid();
        if (!me.empty()) v.push_back(std::move(me));
        for (auto &sid : enumerateLocalAdminMembers()) {
            v.push_back(std::move(sid));
        }
        return v;
    }();
    return cached;
}

bool isWellKnownTrusted(PSID sid)
{
    if (IsWellKnownSid(sid, WinBuiltinAdministratorsSid)) return true;
    if (IsWellKnownSid(sid, WinLocalSystemSid)) return true;
    if (IsWellKnownSid(sid, WinLocalServiceSid)) return true;
    if (IsWellKnownSid(sid, WinNetworkServiceSid)) return true;
    if (IsWellKnownSid(sid, WinCreatorOwnerSid)) return true;
    if (IsWellKnownSid(sid, WinCreatorGroupSid)) return true;
    // System-managed principal families under NT_AUTHORITY (S-1-5-x):
    //   80 — NT SERVICE\* (TrustedInstaller and per-service SIDs)
    //   82 — IIS AppPool identities
    //   83 — NT VIRTUAL MACHINE\*
    //   90 — Window Manager
    // Directories owned by these principals are system-managed and not a peer-write risk.
    PSID_IDENTIFIER_AUTHORITY auth = GetSidIdentifierAuthority(sid);
    UCHAR *subAuthCount = GetSidSubAuthorityCount(sid);
    if (auth && subAuthCount && *subAuthCount >= 1
        && auth->Value[0] == 0 && auth->Value[1] == 0 && auth->Value[2] == 0
        && auth->Value[3] == 0 && auth->Value[4] == 0 && auth->Value[5] == 5) {
        DWORD *sub = GetSidSubAuthority(sid, 0);
        if (sub && (*sub == 80 || *sub == 82 || *sub == 83 || *sub == 90)) return true;
    }
    return false;
}

bool isPrivilegedSid(PSID sid)
{
    if (!sid || !IsValidSid(sid)) return false;
    if (isWellKnownTrusted(sid)) return true;
    for (const auto &buf : trustedSids()) {
        if (EqualSid(sid, const_cast<BYTE *>(buf.data()))) return true;
    }
    return false;
}

// Evaluates effective access for `principal` against `sd` via AuthzAccessCheck,
// which walks the DACL with correct DENY handling.  Returns true if any bit of
// kRiskyWriteMask is granted.
bool principalHasRiskyWrite(AUTHZ_RESOURCE_MANAGER_HANDLE rm, PSECURITY_DESCRIPTOR sd, PSID principal)
{
    AUTHZ_CLIENT_CONTEXT_HANDLE ctx = nullptr;
    LUID zero = {};
    if (!AuthzInitializeContextFromSid(0, principal, rm, nullptr, zero, nullptr, &ctx)) {
        return false;
    }
    auto ctxGuard = wsl::wsScopeGuard([&ctx] { if (ctx) AuthzFreeContext(ctx); });

    AUTHZ_ACCESS_REQUEST req = {};
    req.DesiredAccess = MAXIMUM_ALLOWED;
    ACCESS_MASK granted = 0;
    DWORD err = ERROR_SUCCESS;
    AUTHZ_ACCESS_REPLY reply = {};
    reply.ResultListLength = 1;
    reply.GrantedAccessMask = &granted;
    reply.Error = &err;
    if (!AuthzAccessCheck(0, ctx, &req, nullptr, sd, nullptr, 0, &reply, nullptr)) {
        return false;
    }
    if (err != ERROR_SUCCESS) {
        return false;
    }
    return (granted & kRiskyWriteMask) != 0;
}

// Builds a well-known SID into a self-owning buffer.  Returns empty on failure.
std::vector<BYTE> makeWellKnownSid(WELL_KNOWN_SID_TYPE type)
{
    DWORD size = SECURITY_MAX_SID_SIZE;
    std::vector<BYTE> buf(size);
    if (!CreateWellKnownSid(type, nullptr, buf.data(), &size)) {
        return {};
    }
    buf.resize(size);
    return buf;
}

} // namespace

bool isWritableByOtherUsers(const QString &path)
{
    // UNC paths expose the directory to any network principal that can reach the share.
    // GetNamedSecurityInfoW only inspects the NTFS DACL, not the share-level permissions,
    // and the relevant principal space (network users) is broader than local users anyway.
    // Flag conservatively.
    if (PathIsUNCW(reinterpret_cast<LPCWSTR>(path.utf16()))) {
        return true;
    }

    PACL dacl = nullptr;
    PSID ownerSid = nullptr;
    PSECURITY_DESCRIPTOR sd = nullptr;
    auto sdGuard = wsl::wsScopeGuard([&sd] { if (sd) LocalFree(sd); });

    // GROUP_SECURITY_INFORMATION is requested even though we don't extract the primary
    // group separately: AuthzAccessCheck needs the group set on the SD to resolve
    // CREATOR GROUP references in inherited ACEs.  Don't drop the flag.
    const DWORD status = GetNamedSecurityInfoW(
        reinterpret_cast<LPCWSTR>(path.utf16()),
        SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION,
        &ownerSid, nullptr, &dacl, nullptr, &sd);
    if (status != ERROR_SUCCESS) {
        return false;
    }

    bool risky = false;

    // Owner has implicit WRITE_DAC — a non-privileged owner can rewrite the DACL at will.
    if (ownerSid && !isPrivilegedSid(ownerSid)) {
        risky = true;
    }

    AUTHZ_RESOURCE_MANAGER_HANDLE rm = nullptr;
    auto rmGuard = wsl::wsScopeGuard([&rm] { if (rm) AuthzFreeResourceManager(rm); });

    if (!risky) {
        if (!AuthzInitializeResourceManager(AUTHZ_RM_FLAG_NO_AUDIT, nullptr, nullptr,
                                             nullptr, nullptr, &rm)) {
            // Can't evaluate the DACL without Authz — fail closed rather than silently accept.
            return true;
        }

        // Broad non-privileged principals — any effective write here means unrelated
        // users on this machine can drop files.  Authz evaluates DENYs correctly.
        const WELL_KNOWN_SID_TYPE broad[] = {
            WinWorldSid,              // Everyone (S-1-1-0)
            WinAuthenticatedUserSid,  // S-1-5-11
            WinBuiltinUsersSid,       // BUILTIN\Users
            WinBuiltinGuestsSid,      // BUILTIN\Guests
        };
        for (WELL_KNOWN_SID_TYPE type : broad) {
            if (risky) break;
            std::vector<BYTE> sidBuf = makeWellKnownSid(type);
            if (sidBuf.empty()) continue;
            if (principalHasRiskyWrite(rm, sd, sidBuf.data())) risky = true;
        }

        // Specific non-trusted ALLOW entries that broad-principal queries wouldn't cover
        // (e.g. a specific non-admin user granted write).  Verifying via Authz honors any
        // preceding DENY on the same principal.
        if (!risky && dacl) {
            ACL_SIZE_INFORMATION aclInfo = {};
            if (GetAclInformation(dacl, &aclInfo, sizeof(aclInfo), AclSizeInformation)) {
                for (DWORD i = 0; i < aclInfo.AceCount && !risky; ++i) {
                    void *ace = nullptr;
                    if (!GetAce(dacl, i, &ace) || !ace) continue;
                    ACE_HEADER *hdr = static_cast<ACE_HEADER *>(ace);
                    if (hdr->AceType != ACCESS_ALLOWED_ACE_TYPE) continue;
                    if (hdr->AceFlags & INHERIT_ONLY_ACE) continue;
                    ACCESS_ALLOWED_ACE *allow = static_cast<ACCESS_ALLOWED_ACE *>(ace);
                    if (!(allow->Mask & kRiskyWriteMask)) continue;
                    // SidStart is the documented layout for standard ALLOW ACEs; object
                    // and compressed ACE types are skipped by the AceType check above.
                    PSID aceSid = reinterpret_cast<PSID>(&allow->SidStart);
                    if (isPrivilegedSid(aceSid)) continue;
                    if (principalHasRiskyWrite(rm, sd, aceSid)) risky = true;
                }
            }
        }
    }

    return risky;
}

} // namespace DirectoryPermissions
