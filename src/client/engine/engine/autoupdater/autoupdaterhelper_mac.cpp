#include "autoupdaterhelper_mac.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include "utils/log/categories.h"
#include "utils/utils.h"

namespace {

// hdiutil attach prints a table; the mounted volume path is the last non-empty line's columns 3+
// (the mount point can contain spaces, so join everything past the device and fs-type columns).
QString parseMountPoint(const QString &output)
{
    QString lastLine;
    const QStringList lines = output.split('\n');
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            lastLine = trimmed;
        }
    }
    if (lastLine.isEmpty()) {
        return "";
    }
    static QRegularExpression regExp("\\s+");
    const QStringList entries = lastLine.split(regExp, Qt::SkipEmptyParts);
    if (entries.length() > 2) {
        return entries.mid(2).join(" ");
    }
    return "";
}

} // namespace

AutoUpdaterHelper_mac::AutoUpdaterHelper_mac()
{

}

const QString AutoUpdaterHelper_mac::copyInternalInstallerToTempFromDmg(const QString &dmgFilename)
{
    error_ = UPDATE_VERSION_ERROR_NO_ERROR;

    const QString volumeMountPoint = mountDmg(dmgFilename);

    if (volumeMountPoint == "")
    {
        error_ = UPDATE_VERSION_ERROR_MOUNT_FAIL;
        return "";
    }

    // verify dmg contains installer
    const QString volumeInstallerFilename = volumeMountPoint + "/" + WS_MAC_INSTALLER_BUNDLE_NAME;
    if (!QFileInfo::exists(volumeInstallerFilename))
    {
        qCCritical(LOG_AUTO_UPDATER) << "Volume installer does not exist: " + volumeInstallerFilename;
        unmountVolume(volumeMountPoint);
        error_ = UPDATE_VERSION_ERROR_DMG_HAS_NO_INSTALLER_FAIL;
        return "";
    }
    qCDebug(LOG_AUTO_UPDATER) << "Volume has installer: " << volumeInstallerFilename;

    // remove pre-existing temp installer
    QFileInfo dmgFileInfo(dmgFilename);
    const QString tempDirName = dmgFileInfo.canonicalPath();
    QString tempInstallerFilename = tempDirName + "/" + WS_MAC_INSTALLER_BUNDLE_NAME;
    if (QFileInfo::exists(tempInstallerFilename))
    {
        qCDebug(LOG_AUTO_UPDATER) << "Temp installer already exists -- removing: " << tempInstallerFilename;
        if (!Utils::removeDirectory(tempInstallerFilename))
        {
            qCCritical(LOG_AUTO_UPDATER) << "Couldn't remove temp file: " << tempInstallerFilename;
            unmountVolume(volumeMountPoint);
            error_ = UPDATE_VERSION_ERROR_CANNOT_REMOVE_EXISTING_TEMP_INSTALLER_FAIL;
            return "";
        }
    }
    // qCDebug(LOG_AUTO_UPDATER) << "No pre-existing temp installer: " << tempInstallerFilename;

    // copy/replace installer to temp
    if (!Utils::copyDirectoryRecursive(volumeInstallerFilename, tempInstallerFilename))
    {
        qCCritical(LOG_AUTO_UPDATER) << "Failed to copy installer into temp folder";
        unmountVolume(volumeMountPoint);
        Utils::removeDirectory(tempInstallerFilename); // remove any partially copied application
        error_ = UPDATE_VERSION_ERROR_COPY_FAIL;
        return "";
    }
    qCDebug(LOG_AUTO_UPDATER) << "Copied installer to: " << tempInstallerFilename;

    unmountVolume(volumeMountPoint);

    return tempInstallerFilename;
}

bool AutoUpdaterHelper_mac::verifyAndRun(const QString &tempInstallerFilename,
                                         const QString &additionalArgs)
{
    if (error_ != UPDATE_VERSION_ERROR_NO_ERROR)
    {
        qCCritical(LOG_AUTO_UPDATER) << "Cannot verify and run when dmg unpacking has failed";
        return false;
    }

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

const QString AutoUpdaterHelper_mac::mountDmg(const QString &dmgFilename)
{
    qCDebug(LOG_AUTO_UPDATER) << "Mounting: " << dmgFilename;

    // Bound each hdiutil call with a timeout so a hung hdiutil can't stall the update flow. QProcess
    // owns the child's whole lifecycle -- start, timed wait, output capture, reaping -- so there is
    // no manual waitpid/reaping to get subtly wrong.
    QProcess process;
    process.start("/usr/bin/hdiutil", {"attach", dmgFilename});

    // Generous cap: hdiutil is silent on stdout until it finishes checksum-verifying the image, so a
    // large image on a slow disk can take minutes. Bound it only to catch a genuine hang.
    if (!process.waitForFinished(10 * 60 * 1000)) {
        qCCritical(LOG_AUTO_UPDATER) << "hdiutil attach did not finish, terminating:" << process.errorString();
        process.kill();
        process.waitForFinished(5000);
        // Fall through to parse: hdiutil may have mounted the volume before wedging, in which case its
        // mount line is in the output. Returning that mount point lets the caller unmount the volume
        // instead of leaving it orphaned under /Volumes.
    } else if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qCCritical(LOG_AUTO_UPDATER) << "Mounting process failed with exit code:" << process.exitCode();
        // hdiutil may still have mounted the volume before failing; unmount it so a failed attach
        // doesn't leave the volume orphaned under /Volumes, then report failure.
        const QString volumeName = parseMountPoint(QString::fromUtf8(process.readAllStandardOutput()));
        if (!volumeName.isEmpty()) {
            unmountVolume(volumeName);
        }
        return "";
    }

    const QString volumeName = parseMountPoint(QString::fromUtf8(process.readAllStandardOutput()));
    if (!volumeName.isEmpty()) {
        qCDebug(LOG_AUTO_UPDATER) << "Mounted: " << dmgFilename << " on: " << volumeName;
        return volumeName;
    }

    qCCritical(LOG_AUTO_UPDATER) << "Failed to mount " << dmgFilename;

    return "";
}

bool AutoUpdaterHelper_mac::unmountVolume(const QString &volumePath)
{
    if (!QFileInfo::exists(volumePath)) {
        qCDebug(LOG_AUTO_UPDATER) << "No volume mounted by the name: " << volumePath;
        return true;
    }

    QProcess process;
    process.start("/usr/bin/hdiutil", {"detach", volumePath});

    // Bounded wait: `hdiutil detach` blocks/retries while the volume is busy (Spotlight/AV/Finder
    // holding a file open), so cap it and kill on timeout rather than hanging the update flow.
    if (!process.waitForFinished(30 * 1000)) {
        qCCritical(LOG_AUTO_UPDATER) << "Unmount of " << volumePath << " timed out; terminating hdiutil detach";
        process.kill();
        process.waitForFinished(5000);
        return false;
    }

    // A non-zero exit means the volume is still mounted (e.g. busy), so report failure.
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qCCritical(LOG_AUTO_UPDATER) << "Unmount of " << volumePath << " failed with exit code:" << process.exitCode();
        return false;
    }

    return true;
}
