#include "autoupdaterhelper_mac.h"

#include <QCoreApplication>
#include <QProcess>
#include "utils/log/categories.h"
#include "utils/utils.h"

AutoUpdaterHelper_mac::AutoUpdaterHelper_mac() : error_(UPDATE_VERSION_ERROR_NO_ERROR)
{
}

bool AutoUpdaterHelper_mac::verifyAndRun(const QString &tempInstallerFilename,
                                         const QString &additionalArgs)
{
    // Signature verification already happened in the privileged helper on the staged copy
    // at /Library/Application Support/Windscribe/update. That location is root:wheel and not
    // user-writable, so re-verifying here would be redundant — the bytes can't have changed.
    QString appFolder = QCoreApplication::applicationDirPath();
    qCDebug(LOG_AUTO_UPDATER) << "Starting installer with install location: " << appFolder;

    // start installer
    // use non-static start detached to prevent output from polluting cli
    QStringList args;
    args << "-q";
    args << appFolder;
    if (!additionalArgs.isEmpty())
        args.append(additionalArgs.split(" "));

    QProcess process;
    process.setProgram(tempInstallerFilename + "/" + WS_MAC_INSTALLER_INNER_BINARY);
    process.setArguments(args);
    process.setWorkingDirectory(appFolder);
    process.setStandardOutputFile(QProcess::nullDevice());
    process.setStandardErrorFile(QProcess::nullDevice());
    qint64 pid;
    if (!process.startDetached(&pid))
    {
        qCCritical(LOG_AUTO_UPDATER) << "Could not start installer process - Removing unsigned installer";
        if (!Utils::removeDirectory(tempInstallerFilename))
        {
            qCCritical(LOG_AUTO_UPDATER) << "Could not remove unsigned installer";
        }
        error_ = UPDATE_VERSION_ERROR_START_INSTALLER_FAIL;
        return false;
    }

    return true;
}

UPDATE_VERSION_ERROR AutoUpdaterHelper_mac::error()
{
    return error_;
}
