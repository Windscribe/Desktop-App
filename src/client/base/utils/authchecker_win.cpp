#include "authchecker_win.h"

#include <Windows.h>
#include <shellapi.h>

#include "utils/log/categories.h"

#include "utils/winutils.h"

AuthChecker_win::AuthChecker_win(QObject *parent) : IAuthChecker(parent)
{
}

AuthCheckerError AuthChecker_win::authenticate()
{
    /* Design Note:
    We perform the following logic to confirm the user has knowledge of admin credentials to avoid this
    edge case privilege escalation exploit:

    1. Start app as low privilege user.
    2. Select a configuration directory that you control.
    3. Add a script that is executed by OpenVPN into a custom config and connect.
    4. Script runs as admin user since the helper runs windscribeopenvpn.exe as admin.

    We have logic in the helper to remove potentially harmful scripts... but just in case that is ever
    removed, we have opted to keep the below logic alive.
     */

    const QString cmd = WinUtils::getSystemDir() + QString("\\whoami.exe");

    SHELLEXECUTEINFO shExInfo;
    memset(&shExInfo, 0, sizeof(shExInfo));
    shExInfo.cbSize = sizeof(shExInfo);
    shExInfo.fMask = SEE_MASK_DEFAULT;
    shExInfo.lpVerb = L"runas";
    shExInfo.lpFile = qUtf16Printable(cmd);
    shExInfo.nShow = SW_HIDE;

    const auto result = ::ShellExecuteEx(&shExInfo);

    if (!result) {
        qCWarning(LOG_AUTH_HELPER) << "AuthChecker_win::authenticate() ShellExecuteEx failed:" << ::GetLastError();
        return AuthCheckerError::AUTH_AUTHENTICATION_ERROR;
    }

    return AuthCheckerError::AUTH_NO_ERROR;
}
