#ifndef FILENAMES_H
#define FILENAMES_H

#include <QString>

const QString INSTALLER_FILENAME_MAC = "WindscribeInstaller";
const QString INSTALLER_FILENAME_MAC_APP = INSTALLER_FILENAME_MAC + ".app";
const QString INSTALLER_FILENAME_MAC_DMG = INSTALLER_FILENAME_MAC + ".dmg";

const QString INSTALLER_INNER_BINARY_MAC = "Contents/MacOS/installer";

const QString LAUNCHER_BUNDLE_ID = "com.windscribe.launcher.macos";
const QString GUI_BUNDLE_ID = "com.windscribe.gui.macos";
const QString HELPER_BUNDLE_ID = "com.windscribe.helper.macos";

const QString HELPER_BUNDLE_ID_PATH_FROM_ENGINE = "Contents/Library/LaunchServices/" + HELPER_BUNDLE_ID;


#endif // FILENAMES_H
