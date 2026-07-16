#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

class QFileSystemWatcher;

// Watches a single file, surviving deletion and atomic temp-file-rename replacement, and emits
// changed() once writes settle.
class FileChangeWatcher : public QObject
{
    Q_OBJECT
public:
    explicit FileChangeWatcher(const QString &path, QObject *parent = nullptr);

signals:
    void changed();

private slots:
    void onFileChanged();
    void onDirectoryChanged();
    void onReloadTimeout();

private:
    bool updateFileWatchingState();

    QFileSystemWatcher *watcher_ = nullptr;
    QTimer reloadTimer_;
    QString path_;
};
