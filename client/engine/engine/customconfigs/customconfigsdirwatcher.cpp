#include "customconfigsdirwatcher.h"

#include <QDir>
#include <QStandardPaths>
#include "utils/log/logger.h"

CustomConfigsDirWatcher::CustomConfigsDirWatcher(QObject *parent, const QString &path)
    : QObject(parent), path_(path)
{
    dirWatcher_.addPath(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    dirWatcher_.addPath(path_);

    connect(&dirWatcher_, &QFileSystemWatcher::directoryChanged, this, &CustomConfigsDirWatcher::onDirectoryChanged);
    connect(&dirWatcher_, &QFileSystemWatcher::fileChanged, this, &CustomConfigsDirWatcher::onFileChanged);

    connect(&emitTimer_, &QTimer::timeout, this, &CustomConfigsDirWatcher::onEmitTimer);
    emitTimer_.setSingleShot(true);

    checkFiles(false, false);
}

QString CustomConfigsDirWatcher::curDir() const
{
    return path_;
}

QStringList CustomConfigsDirWatcher::curFiles() const
{
    return curFiles_;
}

void CustomConfigsDirWatcher::onDirectoryChanged(const QString & /*path*/)
{
    checkFiles(true, false);
}

void CustomConfigsDirWatcher::onFileChanged(const QString & /*path*/)
{
    checkFiles(true, true);
}

void CustomConfigsDirWatcher::onEmitTimer()
{
    emit dirChanged();
}

void CustomConfigsDirWatcher::checkFiles(bool bWithEmitSignal, bool bFileChanged)
{
    // remove prev files from watch paths
    for (const QString &filename : qAsConst(curFiles_))
    {
        dirWatcher_.removePath(path_ + "/" + filename);
    }

    QDir dir(path_);
    QStringList filters;
    filters << "*.ovpn" << "*.conf";
    dir.setNameFilters(filters);
    QStringList fileList = dir.entryList(QDir::Files);

    QStringList newFileList;
    for (const QString &filename : fileList)
    {
        // skip our temp file
        if (filename == "windscribe_temp_config.ovpn")
        {
            continue;
        }
        newFileList << filename;

        dirWatcher_.addPath(path_ + "/" + filename);
    }

    if ((!bFileChanged && newFileList != curFiles_) || bFileChanged)
    {

        curFiles_ = newFileList;
        if (bWithEmitSignal)
        {
            emitTimer_.start(EMIT_TIMEOUT);
        }
    }
}
