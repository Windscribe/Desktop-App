#include "iniwatcher.h"

#include <QSettings>

#include "utils/filechangewatcher.h"

IniWatcher::IniWatcher(QObject *parent) : QObject(parent)
{
    watcher_ = new FileChangeWatcher(QSettings(WS_SETTINGS_ORG, WS_SETTINGS_CLI).fileName(), this);
    connect(watcher_, &FileChangeWatcher::changed, this, &IniWatcher::changed);
}
