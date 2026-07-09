#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

class QFileSystemWatcher;

// Watches the CLI preferences ini file and emits changed() once writes settle, so the running
// service can reload it automatically instead of the user invoking 'windscribe-cli preferences reload'.
class IniWatcher : public QObject
{
    Q_OBJECT
public:
    explicit IniWatcher(QObject *parent = nullptr);

signals:
    void changed();

private slots:
    void onFileChanged();
    void onDirectoryChanged();
    void onReloadTimeout();

private:
    void updateFileWatchingState();

    QFileSystemWatcher *watcher_ = nullptr;
    QTimer reloadTimer_;
    QString path_;
};
