#include "paths.h"

#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

#if defined(Q_OS_WIN)
#include <Windows.h>
#include <shlobj.h>
#endif

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
    return clientLogFolder() + addPreviousSuffix("/client.log", previous);
}

QString paths::cliLogLocation(bool previous)
{
    return clientLogFolder() + addPreviousSuffix("/cli.log", previous);
}

void paths::deleteOldUnusedLogs()
{
    QString appDataPath = clientLogFolder();
    QFile::remove(appDataPath + "/log_gui.txt");
    QFile::remove(appDataPath + "/prev_log_gui.txt");
    QFile::remove(appDataPath + "/ping_log.txt");
    QFile::remove(appDataPath + "/ping_log_custom_configs.txt");
    QFile::remove(appDataPath + "/log_singleappinstanceguard.txt");

#ifdef WS_IS_WINDSCRIBE
    // remove Windscribe folder which is no longer used since version 2.xx
    // now we use only Windscribe2 folder
    QDir dir(appDataPath);
    if (dir.cdUp()) {
        if (dir.cd("Windscribe"))
            dir.removeRecursively();
    }
#endif
}

QString paths::serviceLogLocation(bool previous)
{
#if defined(Q_OS_WIN)
    return serviceLogFolder() + addPreviousSuffix("/" WS_PRODUCT_NAME_LOWER "_service.log", previous);
#else
    return serviceLogFolder() + addPreviousSuffix("/helper.log", previous);
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
    // The installer writes its log incrementally to %ProgramData%\Windscribe, where it
    // survives an installer crash and removal of the install folder during an upgrade.
    // It no longer places a copy in the install folder.
    PWSTR programData = nullptr;
    const HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_ProgramData, KF_FLAG_DEFAULT, nullptr, &programData);
    // Per the SHGetKnownFolderPath docs, the output buffer must be freed even on failure.
    if (FAILED(hr)) {
        ::CoTaskMemFree(programData);
        return "";
    }
    const QString location = QString::fromWCharArray(programData) + "/Windscribe/installer.log";
    ::CoTaskMemFree(programData);
    return location;
#endif
}

QString paths::clientLogFolder()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

QString paths::serviceLogFolder()
{
#if defined(Q_OS_LINUX)
    return WS_LINUX_LOG_DIR;
#elif defined(Q_OS_MACOS)
    return "/Library/Logs/" WS_MAC_HELPER_BUNDLE_ID;
#else
    return qApp->applicationDirPath();
#endif
}

}  // namespace log_utils
