#include "authchecker_linux.h"

#include <QCoreApplication>
#include "utils/logger.h"
#include "utils/executable_signature/executable_signature.h"

AuthChecker_linux::AuthChecker_linux(QObject *parent) : IAuthChecker(parent)
{
    process_ = new QProcess(this);
}

AuthCheckerError AuthChecker_linux::authenticate()
{
#ifdef QT_DEBUG
    QString authHelperPath = "/usr/bin/windscribe-authhelper";
#else
    QString appDir = QCoreApplication::applicationDirPath();
    QString authHelperPath = appDir + "/windscribe-authhelper";
#endif

    // TODO: uncomment this verify check once the following are complete:
    // * installer installs windscribe-authhelper and policy file
    // * build_all script builds and signs windscribe-authhelper
    if (!ExecutableSignature::verify(authHelperPath))
    {
        qCDebug(LOG_AUTH_HELPER) << "Failed to verify AuthHelper, executable may be corrupted";
        return AuthCheckerError::HELPER_ERROR;
    }

    QStringList args;
    args << authHelperPath;

    // qCDebug(LOG_AUTH_HELPER) << "Authenticating...";
    process_->start("/usr/bin/pkexec", args);
    process_->waitForFinished(-1);

    if (process_->exitStatus() == QProcess::NormalExit
        && process_->exitCode() == 0
        && process_->error() == QProcess::UnknownError)
    {
        return AuthCheckerError::NO_ERROR;
    }

    qCDebug(LOG_AUTH_HELPER) << "Failed to authenticate: " << process_->exitStatus()
                             << ", code: " << process_->exitCode()
                             << ", error: " << process_->error();
    return AuthCheckerError::AUTHENTICATION_ERROR;
}

