#include "filechangewatcher.h"

#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>

#include <filesystem>

#include "log/categories.h"

FileChangeWatcher::FileChangeWatcher(const QString &path, QObject *parent) : QObject(parent), path_(path)
{
    reloadTimer_.setSingleShot(true);
    connect(&reloadTimer_, &QTimer::timeout, this, &FileChangeWatcher::onReloadTimeout);

    // QFileSystemWatcher::addPath() silently fails if the directory does not exist yet, and on a fresh
    // profile nothing has written the file yet. Create it (create_directories is a no-op if it
    // already exists) so the watch below always takes hold.
    const QString dirPath = QFileInfo(path_).absolutePath();
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(dirPath.toStdString()), ec);

    watcher_ = new QFileSystemWatcher(this);
    // Always watch the directory so we still get notified when the file is created or replaced via
    // an atomic temp-file rename, even though that drops a bare file watch.
    if (!watcher_->addPath(dirPath)) {
        qCWarning(LOG_BASIC) << "Could not watch directory for changes:" << dirPath;
    }
    connect(watcher_, &QFileSystemWatcher::fileChanged, this, &FileChangeWatcher::onFileChanged);
    connect(watcher_, &QFileSystemWatcher::directoryChanged, this, &FileChangeWatcher::onDirectoryChanged);

    updateFileWatchingState();
}

void FileChangeWatcher::onFileChanged()
{
    // Debounce: a single save often arrives as several filesystem events, and reading mid-write could
    // parse a truncated file. Wait until writes stop before signalling a reload.
    reloadTimer_.start(1000);
}

void FileChangeWatcher::onDirectoryChanged()
{
    // Only signal when the watched file itself appeared, was replaced, or was removed -- the directory
    // may be shared with unrelated files (logs etc.) whose churn is not a change of this file.
    if (updateFileWatchingState()) {
        reloadTimer_.start(1000);
    }
}

void FileChangeWatcher::onReloadTimeout()
{
    emit changed();
}

bool FileChangeWatcher::updateFileWatchingState()
{
    // QFileSystemWatcher drops the file from its watch list when the file is deleted or atomically
    // replaced (temp-file rename). Re-add it whenever it exists but isn't being watched, so an
    // in-place edit after such a replace is still caught.
    if (QFile::exists(path_)) {
        if (!watcher_->files().contains(path_)) {
            if (!watcher_->addPath(path_)) {
                qCWarning(LOG_BASIC) << "Could not watch file for changes:" << path_;
            }
            return true;
        }
    } else if (watcher_->files().contains(path_)) {
        // Deleted while watched; fileChanged does not fire for this on every platform.
        watcher_->removePath(path_);
        return true;
    }
    return false;
}
