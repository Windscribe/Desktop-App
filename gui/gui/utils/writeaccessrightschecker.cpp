#include "writeaccessrightschecker.h"

#include <QTemporaryFile>

#if defined(Q_OS_WIN)
#include <Windows.h>
#else
#include <unistd.h>
#endif

WriteAccessRightsChecker::WriteAccessRightsChecker(const QString &dirname) :
    QTemporaryFile(dirname + "/FileAccessTest_XXXXXX"), is_writeable_(false)
{
    if (dirname.isEmpty())
        return;

    bool is_elevated_check_ok = false;

    if (isElevated()) {
        // Under elevated privileges, the directory will be reported as writeable, even if it is not
        // really writeable for users. Restrict privileges temporarily to check access.
#if defined(Q_OS_WIN)
        HANDLE process_token = 0;
        if (OpenProcessToken(GetCurrentProcess(),
                             TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_QUERY,
                             &process_token)) {
            SID_AND_ATTRIBUTES admin_sid_and_attr;
            SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
            ZeroMemory(&admin_sid_and_attr, sizeof(admin_sid_and_attr));
            if (AllocateAndInitializeSid(
                &nt_authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0,
                0, 0, &admin_sid_and_attr.Sid)) {
                HANDLE restricted_token = 0;
                if (CreateRestrictedToken(process_token, 0, 1, &admin_sid_and_attr, 0, nullptr, 0,
                    nullptr, &restricted_token)) {
                    if (ImpersonateLoggedOnUser(restricted_token)) {
                        testWrite();
                        RevertToSelf();
                        is_elevated_check_ok = true;
                    }
                    CloseHandle(restricted_token);
                }
                FreeSid(admin_sid_and_attr.Sid);
            }
            CloseHandle(process_token);
        }
#else  // Q_OS_WIN
        const int current_uid = getuid();
        const int saved_euid = geteuid();
        if (current_uid != 0) {
            // We are running the app as root, revert to the real uid temporarily.
            if (seteuid(current_uid) != -1) {
                testWrite();
                seteuid(saved_euid);
                is_elevated_check_ok = true;
            }
        }
#endif  // Q_OS_WIN
    }
    // Generic user privileges mean that we can do a simple test. We can also make it to here if we
    // failed to check access under restricted privileges.
    if (!is_elevated_check_ok)
        testWrite();
}

bool WriteAccessRightsChecker::isWriteable() const
{
    return is_writeable_;
}

bool WriteAccessRightsChecker::isElevated() const
{
    bool result = false;
#if defined(Q_OS_WIN)
    HANDLE process_token = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &process_token)) {
        TOKEN_ELEVATION token_elevation_data;
        DWORD token_elevation_size = sizeof(token_elevation_data);
        if (GetTokenInformation(process_token, TokenElevation, &token_elevation_data,
                                sizeof(token_elevation_data), &token_elevation_size))
            result = token_elevation_data.TokenIsElevated != 0;
        CloseHandle(process_token);
    }
#else  // Q_OS_WIN
    if (geteuid() == 0)
        result = true;
#endif  // Q_OS_WIN
    return result;
}

void WriteAccessRightsChecker::testWrite()
{
    is_writeable_ = open(QIODevice::WriteOnly);
}
