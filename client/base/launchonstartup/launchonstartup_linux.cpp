#include "launchonstartup_linux.h"

#include <QDir>
#include <QFile>
#include "utils/log/categories.h"

void LaunchOnStartup_linux::setLaunchOnStartup(bool enable)
{
#ifdef CLI_ONLY
    int err = system("systemctl --user enable windscribe");
    if (err != 0) {
        qCWarning(LOG_BASIC) << "Could not enable user service for launch on startup";
    }
#else
    if (enable) {
        if (QFile::exists("/etc/windscribe/autostart/windscribe.desktop"))
        {
            QString destDir = QDir::homePath() + "/.config/autostart";
            QDir dir(destDir);
            dir.mkpath(destDir);
            QString destFile = destDir + "/windscribe.desktop";
            QFile::remove(destFile);
            QFile::link("/etc/windscribe/autostart/windscribe.desktop", destFile);
        } else {
            qCWarning(LOG_BASIC) << "LaunchOnStartup_linux: File /etc/windscribe/autostart/windscribe.desktop doesn't exist";
        }
    } else {
        QString destFile = QDir::homePath() + "/.config/autostart/windscribe.desktop";
        QFile::remove(destFile);
    }
#endif
}

