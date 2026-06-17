#include "launchonstartup_linux.h"

#include <cerrno>
#include <system_error>
#include <unistd.h>

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
    const QString destDir = QDir::homePath() + "/.config/autostart";
    const QString destFile = destDir + "/" WS_PRODUCT_NAME_LOWER ".desktop";

    if (enable) {
        const QString desktopFile = QString(WS_POSIX_CONFIG_DIR "/autostart/" WS_PRODUCT_NAME_LOWER ".desktop");
        if (!QFile::exists(desktopFile)) {
            qCWarning(LOG_BASIC) << "LaunchOnStartup_linux: File" << desktopFile << "doesn't exist";
            return;
        }

        QDir dir(destDir);
        dir.mkpath(destDir);

        // Install the symlink atomically: create it under a temporary name and rename() it into place.
        // A plain remove-then-create leaves a window with no autostart entry that becomes permanent if
        // the process is killed (e.g. during OS shutdown) before the link is recreated.
        const QString tmpFile = destFile + ".tmp." + QString::number(::getpid());
        QFile::remove(tmpFile);
        if (::symlink(desktopFile.toLocal8Bit().constData(), tmpFile.toLocal8Bit().constData()) != 0) {
            const int err = errno;
            qCWarning(LOG_BASIC) << "LaunchOnStartup_linux: could not create autostart symlink" << tmpFile << ":" << QString::fromStdString(std::generic_category().message(err));
            return;
        }
        if (::rename(tmpFile.toLocal8Bit().constData(), destFile.toLocal8Bit().constData()) != 0) {
            const int err = errno;
            qCWarning(LOG_BASIC) << "LaunchOnStartup_linux: could not install autostart symlink" << destFile << ":" << QString::fromStdString(std::generic_category().message(err));
            QFile::remove(tmpFile);
        }
    } else {
        QFile::remove(destFile);
    }
#endif
}
