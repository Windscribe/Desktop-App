#include "paths.h"

#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

namespace {
// For example  client.log -> client.1.log
QString addPreviousSuffix(const QString &path, bool bAdd)
{
    if (bAdd) {
        int posDot = path.lastIndexOf('.');
        if (posDot != - 1) {
            QString res = path;
            res.insert(posDot, ".1");
            return res;
        }
    }
    return path;
}
}

namespace log_utils {

QString paths::clientLogLocation(bool previous)
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + addPreviousSuffix("/client.log", previous);
}

QString paths::cliLogLocation(bool previous)
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + addPreviousSuffix("/cli.log", previous);
}

void paths::deleteOldUnusedLogs()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QFile::remove(appDataPath + "/log_gui.txt");
    QFile::remove(appDataPath + "/prev_log_gui.txt");
    QFile::remove(appDataPath + "/ping_log.txt");
    QFile::remove(appDataPath + "/ping_log_custom_configs.txt");
    QFile::remove(appDataPath + "/log_singleappinstanceguard.txt");

    // remove Windscribe folder which is no longer used since version 2.xx
    // now we use only Windcribe2 folder
    QDir dir(appDataPath);
    if (dir.cdUp()) {
        if (dir.cd("Windscribe"))
            dir.removeRecursively();
    }
}

QString paths::serviceLogLocation(bool previous)
{
#if defined(Q_OS_LINUX)
    return "/var/log/windscribe" + addPreviousSuffix("/helper.log", previous);
#elif defined(Q_OS_MACOS)
    return "/Library/Logs/com.windscribe.helper.macos" + addPreviousSuffix("/helper.log", previous);
#else
    return qApp->applicationDirPath() + addPreviousSuffix("/windscribe_service.log", previous);
#endif
}

QString paths::wireguardServiceLogLocation(bool previous)
{
#if defined(Q_OS_WIN)
    return qApp->applicationDirPath() + addPreviousSuffix("/wireguard_service.log", previous);
#else
    return "";
#endif
}

QString paths::installerLogLocation()
{
#if defined(Q_OS_LINUX)
    return "";
#elif defined(Q_OS_MACOS)
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/installer.log";
#else
    return qApp->applicationDirPath() + "/installer.log";
#endif
}

}  // namespace log_utils
