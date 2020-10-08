#include "volumehelper_mac.h"

#include <QFileInfo>
#include <QProcess>
#include "utils/logger.h"

VolumeHelper_mac::VolumeHelper_mac()
{

}

const QString VolumeHelper_mac::mountDmg(const QString &dmgFilename)
{
    qCDebug(LOG_VOLUME_HELPER) << "Mounting: " << dmgFilename;

    // mount
    QStringList args;
    args << "attach";
    args << "" + dmgFilename + "";
    QProcess mountProcess;
    mountProcess.start("/usr/bin/hdiutil", args);
    if (!mountProcess.waitForStarted())
    {
        qCDebug(LOG_VOLUME_HELPER) << "Failed to start mounting process";
        return "";
    }
    if (!mountProcess.waitForFinished())
    {
        qCDebug(LOG_VOLUME_HELPER) << "Mounting process has failed";
        return "";
    }

    // parse output for volume mount point
    const QString mountingOutput = mountProcess.readAll();
    const QList<QString> lines = mountingOutput.split("\n", QString::SkipEmptyParts);
    if (lines.length() > 0)
    {
        const QString lastLine = lines[lines.length()-1];
        QStringList entries = lastLine.split(QRegExp("\\s+"), QString::SkipEmptyParts);

        if (entries.length() > 2)
        {
            QStringList volumeNameList = entries.mid(2);
            QString volumeName = QString(volumeNameList.join(" "));

            qCDebug(LOG_VOLUME_HELPER) << "Mounted: " << dmgFilename << " on: " << volumeName;
            return volumeName;
        }
    }

    const QString hdiutilOutput = mountProcess.readAllStandardError();
    qCDebug(LOG_VOLUME_HELPER) << "Failed to mount " << dmgFilename << " hdiutil error output: " << hdiutilOutput;

    return "";
}

bool VolumeHelper_mac::unmountVolume(const QString &volumePath)
{
    if (!QFileInfo::exists(volumePath))
    {
        qCDebug(LOG_VOLUME_HELPER) << "No volume mounted by the name: " << volumePath;
        return true;
    }

    // unmount
    QStringList args;
    args << "detach";
    args << "" + volumePath + "";
    QProcess unmountProcess;
    unmountProcess.start("/usr/bin/hdiutil", QStringList(args));
    if (!unmountProcess.waitForStarted())
    {
        qCDebug(LOG_VOLUME_HELPER) << "Failed to start unmounting process";
        return false;
    }
    if (!unmountProcess.waitForFinished())
    {
        qCDebug(LOG_VOLUME_HELPER) << "Unmounting process has failed";
        return false;
    }

    return true;
}

const QString VolumeHelper_mac::volumePath(const QString &dmgPathFilename)
{
    QFileInfo dmgFileInfo(dmgPathFilename);
    QString volumeLocation = "/Volumes/" + dmgFileInfo.completeBaseName();
    return volumeLocation;
}
