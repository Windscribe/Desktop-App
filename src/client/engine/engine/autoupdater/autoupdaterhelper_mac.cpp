#include "autoupdaterhelper_mac.h"

#include <QFileInfo>
#include <QProcess>
#include <QCoreApplication>
#include <QRegularExpression>
#include "utils/log/categories.h"
#include "names.h"
#include "utils/utils.h"
#include "utils/executable_signature/executable_signature.h"
#include <boost/process/v1/io.hpp>
#include <boost/process/v1/child.hpp>

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
    const QString volumeInstallerFilename = volumeMountPoint + "/" + INSTALLER_FILENAME_MAC_APP;
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
    QString tempInstallerFilename = tempDirName + "/" + INSTALLER_FILENAME_MAC_APP;
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

    // verify installer
    qCDebug(LOG_AUTO_UPDATER) << "Verifying signature and certificate of installer: " << tempInstallerFilename;
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(tempInstallerFilename.toStdWString()))
    {
        qCCritical(LOG_AUTO_UPDATER) << "Failed to verify signature and certificate of downloaded installer: " << QString::fromStdString(sigCheck.lastError());
        Utils::removeDirectory(tempInstallerFilename);
        error_ = UPDATE_VERSION_ERROR_SIGN_FAIL;
        return false;
    }

    QString appFolder = QCoreApplication::applicationDirPath();
    qCDebug(LOG_AUTO_UPDATER) << "Verified signature and certificate of downloaded installer -- starting with install location: " << appFolder;

    // start installer
    // use non-static start detached to prevent output from polluting cli
    QStringList args;
    args << "-q";
    args << appFolder;
    if (!additionalArgs.isEmpty())
        args.append(additionalArgs.split(" "));

    QProcess process;
    process.setProgram(tempInstallerFilename + "/" + INSTALLER_INNER_BINARY_MAC);
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

    // mount
    // we use boost::process instead of QProcess here because we encountered
    // that QProcess has a bug when waiting for the process to finish on MacOS
    using namespace boost::process::v1;
    ipstream pipe_stream;
    child c("/usr/bin/hdiutil", "attach", dmgFilename.toStdString(), std_out > pipe_stream);

    std::string line;
    std::error_code ec;
    QStringList lines;
    while (pipe_stream && std::getline(pipe_stream, line) && !line.empty())
        lines << QString::fromStdString(line);

    c.wait(ec);

    if (ec.value() != 0) {
        qCCritical(LOG_AUTO_UPDATER) << "Mounting process failed with exit code: " << ec.value();
        return "";
    }

    // parse output for volume mount point
    if (lines.length() > 0)
    {
        const QString lastLine = lines[lines.length()-1];
        static QRegularExpression regExp("\\s+");
        QStringList entries = lastLine.split(regExp, Qt::SkipEmptyParts);

        if (entries.length() > 2)
        {
            QStringList volumeNameList = entries.mid(2);
            QString volumeName = QString(volumeNameList.join(" "));

            qCDebug(LOG_AUTO_UPDATER) << "Mounted: " << dmgFilename << " on: " << volumeName;
            return volumeName;
        }
    }

    qCCritical(LOG_AUTO_UPDATER) << "Failed to mount " << dmgFilename << " hdiutil error output: " << lines;

    return "";
}

bool AutoUpdaterHelper_mac::unmountVolume(const QString &volumePath)
{
    if (!QFileInfo::exists(volumePath))
    {
        qCDebug(LOG_AUTO_UPDATER) << "No volume mounted by the name: " << volumePath;
        return true;
    }

    // unmount
    // we use boost::process instead of QProcess here because we encountered
    // that QProcess has a bug when waiting for the process to finish on MacOS
    using namespace boost::process::v1;
    child c("/usr/bin/hdiutil", "detach", volumePath.toStdString());
    c.wait();

    return true;
}
