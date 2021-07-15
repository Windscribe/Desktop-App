#include "authchecker_linux.h"

#include "utils/logger.h"

AuthChecker_linux::AuthChecker_linux(QObject *parent) : IAuthChecker(parent)
{
    process_ = new QProcess(this);
}

bool AuthChecker_linux::authenticate()
{
    QStringList args;
    args << "/usr/bin/echo";

    qCDebug(LOG_AUTH_HELPER) << "Authenticating...";
    process_->start("/usr/bin/pkexec", args);
    process_->waitForFinished(-1);

    if (process_->exitStatus() == QProcess::NormalExit
        && process_->exitCode() == 0
        && process_->error() == QProcess::UnknownError)
    {
        return true;
    }

    qCDebug(LOG_AUTH_HELPER) << "Failed to authenticate: " << process_->exitStatus()
                             << ", code: " << process_->exitCode()
                             << ", error: " << process_->error();
    return false;
}

