#include "launchonstartup_linux.h"

#include <QDir>
#include <QFile>
#include "utils/logger.h"

void LaunchOnStartup_linux::setLaunchOnStartup(bool enable)
{
#ifdef CLI_ONLY
    int err = system("systemctl --user enable windscribe");
    if (err != 0) {
        qCDebug(LOG_BASIC) << "Could not enable user service for launch on startup";
    }
#else
    if (enable) {
        if (QFile::exists("/usr/share/applications/windscribe.desktop"))
        {
            QString destDir = QDir::homePath() + "/.config/autostart";
            QDir dir(destDir);
            dir.mkpath(destDir);
            QString destFile = destDir + "/windscribe.desktop";
            QFile::remove(destFile);
            QFile::link("/usr/share/applications/windscribe.desktop", destFile);
        } else {
            qCDebug(LOG_BASIC) << "LaunchOnStartup_linux: File /usr/share/applications/windscribe.desktop doesn't exists";
        }
    } else {
        QString destFile = QDir::homePath() + "/.config/autostart/windscribe.desktop";
        QFile::remove(destFile);
    }
#endif
}

