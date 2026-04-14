#include "launchonstartup_linux.h"

#include <QDir>
#include <QFile>
#include "utils/log/categories.h"

void LaunchOnStartup_linux::setLaunchOnStartup(bool enable)
{
#ifdef CLI_ONLY
    int err = system("systemctl --user enable " WS_PRODUCT_NAME_LOWER);
    if (err != 0) {
        qCWarning(LOG_BASIC) << "Could not enable user service for launch on startup";
    }
#else
    if (enable) {
        QString desktopFile = QString(WS_POSIX_CONFIG_DIR "/autostart/" WS_PRODUCT_NAME_LOWER ".desktop");
        if (QFile::exists(desktopFile))
        {
            QString destDir = QDir::homePath() + "/.config/autostart";
            QDir dir(destDir);
            dir.mkpath(destDir);
            QString destFile = destDir + "/" WS_PRODUCT_NAME_LOWER ".desktop";
            QFile::remove(destFile);
            QFile::link(desktopFile, destFile);
        } else {
            qCWarning(LOG_BASIC) << "LaunchOnStartup_linux: File" << desktopFile << "doesn't exist";
        }
    } else {
        QString destFile = QDir::homePath() + "/.config/autostart/" WS_PRODUCT_NAME_LOWER ".desktop";
        QFile::remove(destFile);
    }
#endif
}

