#include "authchecker_win.h"

#include <QCoreApplication>
#include "utils/winutils.h"
#include "utils/logger.h"
#include "utils/executable_signature/executable_signature.h"

AuthChecker_win::AuthChecker_win(QObject *parent) : IAuthChecker(parent)
{

}

AuthCheckerError AuthChecker_win::authenticate()
{
    QString appDir = QCoreApplication::applicationDirPath();

    ExecutableSignature sigCheck;

    QString comServerPath = appDir + "/ws_com_server.exe";
    if (!sigCheck.verify(comServerPath.toStdWString()))
    {
        qCDebug(LOG_AUTH_HELPER) << "Could not verify " << comServerPath << ". File may be corrupted. " << sigCheck.lastError();
        return AuthCheckerError::HELPER_ERROR;
    }

    QString comDllPath    = appDir + "/ws_com.dll";
    if (!sigCheck.verify(comDllPath.toStdWString()))
    {
        qCDebug(LOG_AUTH_HELPER) << "Could not verify " << comDllPath << ". File may be corrupted. " << sigCheck.lastError();
        return AuthCheckerError::HELPER_ERROR;
    }

    QString comStubPath   = appDir + "/ws_proxy_stub.dll";
    if (!sigCheck.verify(comStubPath.toStdWString()))
    {
        qCDebug(LOG_AUTH_HELPER) << "Could not verify " << comStubPath << ". File may be corrupted. " << sigCheck.lastError();
        return AuthCheckerError::HELPER_ERROR;
    }
    return WinUtils::authorizeWithUac() ?
                AuthCheckerError::NO_ERROR :
                AuthCheckerError::AUTHENTICATION_ERROR;
}
