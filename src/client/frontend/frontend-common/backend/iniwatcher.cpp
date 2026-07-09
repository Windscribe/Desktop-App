#include "iniwatcher.h"

#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QSettings>

#include <filesystem>

IniWatcher::IniWatcher(QObject *parent) : QObject(parent)
{
    path_ = QSettings(WS_SETTINGS_ORG, WS_SETTINGS_CLI).fileName();

    reloadTimer_.setSingleShot(true);
    connect(&reloadTimer_, &QTimer::timeout, this, &IniWatcher::onReloadTimeout);

    // QFileSystemWatcher::addPath() silently fails if the directory does not exist yet, and on a fresh
    // profile nothing has written the conf file yet. Create it (create_directories is a no-op if it
    // already exists) so the watch below always takes hold.
    const std::filesystem::path dir = QFileInfo(path_).absolutePath().toStdString();
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    watcher_ = new QFileSystemWatcher(this);
    // Always watch the directory so we still get notified when the file is created or replaced via
    // an atomic temp-file rename (which QSettings does), even though that drops a bare file watch.
    watcher_->addPath(QFileInfo(path_).absolutePath());
    connect(watcher_, &QFileSystemWatcher::fileChanged, this, &IniWatcher::onFileChanged);
    connect(watcher_, &QFileSystemWatcher::directoryChanged, this, &IniWatcher::onDirectoryChanged);

    updateFileWatchingState();
}

void IniWatcher::onFileChanged()
{
    // Debounce: a single save often arrives as several filesystem events, and reading mid-write could
    // parse a truncated file. Wait until writes stop before signalling a reload.
    reloadTimer_.start(1000);
}

void IniWatcher::onDirectoryChanged()
{
    updateFileWatchingState();
    reloadTimer_.start(1000);
}

void IniWatcher::onReloadTimeout()
{
    emit changed();
}

void IniWatcher::updateFileWatchingState()
{
    // QFileSystemWatcher drops the file from its watch list when the file is deleted or atomically
    // replaced (temp-file rename, which QSettings uses). Re-add it whenever it exists but isn't being
    // watched, so an in-place edit after such a replace is still caught.
    if (QFile::exists(path_) && !watcher_->files().contains(path_)) {
        watcher_->addPath(path_);
    }
}
