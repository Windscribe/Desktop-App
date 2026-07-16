#pragma once

#include <QObject>

class FileChangeWatcher;

// Watches the CLI preferences ini file and emits changed() once writes settle, so the running
// service can reload it automatically instead of the user invoking 'windscribe-cli preferences reload'.
class IniWatcher : public QObject
{
    Q_OBJECT
public:
    explicit IniWatcher(QObject *parent = nullptr);

signals:
    void changed();

private:
    FileChangeWatcher *watcher_ = nullptr;
};
