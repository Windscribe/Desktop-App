#include "customconfigsdirwatcher.h"

#include <QDir>
#include <QStandardPaths>
#include "Utils/logger.h"

CustomConfigsDirWatcher::CustomConfigsDirWatcher(QObject *parent, const QString &path) : QObject(parent)
{
    path_ = path;

    dirWatcher_.addPath(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    dirWatcher_.addPath(path_);

    connect(&dirWatcher_, SIGNAL(directoryChanged(QString)), SLOT(onDirectoryChanged(QString)));
    connect(&dirWatcher_, SIGNAL(fileChanged(QString)), SLOT(onFileChanged(QString)));

    connect(&emitTimer_, SIGNAL(timeout()), SLOT(onEmitTimer()));
    emitTimer_.setSingleShot(true);

    checkFiles(false, false);
}

QString CustomConfigsDirWatcher::curDir() const
{
    return path_;
}

void CustomConfigsDirWatcher::onDirectoryChanged(const QString &path)
{
    checkFiles(true, false);
}

void CustomConfigsDirWatcher::onFileChanged(const QString &path)
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
    Q_FOREACH(const QString &filename, curFiles_)
    {
        dirWatcher_.removePath(path_ + "/" + filename);
    }

    QDir dir(path_);
    QStringList filters;
    filters << "*.ovpn";
    dir.setNameFilters(filters);
    QStringList fileList = dir.entryList(QDir::Files);

    QStringList newFileList;
    Q_FOREACH(const QString &filename, fileList)
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
